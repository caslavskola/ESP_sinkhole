#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "esp_log.h"
#include "esp_err.h"

#include "dns.h"
#include "blocklist.h"
#include "statistics.c"
#include "secrets.h"


#define DNS_PORT 53
#define DNS_BUFFER_SIZE 2048
#define DNS_TIMEOUT_SECONDS 2


static const char *TAG = "DNS";


static int dns_forward(uint8_t *request, int request_length, uint8_t *response)
{
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socket_fd < 0)
        return -1;


    struct sockaddr_in server;

    memset( &server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(DNS_UPSTREAM_PORT);

    if (inet_pton(AF_INET, DNS_UPSTREAM_PRIMARY, &server.sin_addr) != 1){
        close(socket_fd);
        return -1;
    }

    int sent = sendto( socket_fd, request, request_length, 0, (struct sockaddr *)&server, sizeof(server));

    if (sent < 0){
    close(socket_fd);
    return -1;
    }

    struct timeval timeout;

    timeout.tv_sec = DNS_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;


    setsockopt( socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    int response_length;

    response_length = recv( socket_fd, response, DNS_BUFFER_SIZE, 0);

    close(socket_fd);

    return response_length;
}


static bool dns_get_domain(uint8_t *packet, int length, char *domain, size_t domain_size)
{
    int position = 12;
    size_t output = 0;


    if (length < 13)
        return false;


    while (position < length)
    {
        uint8_t label_length;

        label_length = packet[position++];


        if (label_length == 0)
            break;

        if (label_length > 63
            || position + label_length > length
            || output + label_length + 1 >= domain_size)
            return false;


        for (int i = 0; i < label_length; i++)
            domain[output++] = packet[position++];

        domain[output++] = '.';
    }


    if (output == 0)
        return false;


    domain[output - 1] = '\0';

    return true;
}


static void dns_make_block_response(uint8_t *packet)
{
    /*
     * Return NXDOMAIN.
     */

    packet[2] |= 0x80;
    packet[3] |= 0x03;


    packet[6] = 0;
    packet[7] = 0;
}


static void dns_task(void *parameter)
{
    static uint8_t request[DNS_BUFFER_SIZE];
    static uint8_t response[DNS_BUFFER_SIZE];

    int socket_fd;

    socket_fd = socket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP
    );


    if (socket_fd < 0){
        ESP_LOGE(TAG, "Socket creation failed");
        vTaskDelete(NULL);
        return;
    }


    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));


    address.sin_family = AF_INET;
    address.sin_port = htons(DNS_PORT);
    address.sin_addr.s_addr = INADDR_ANY;


    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        ESP_LOGE(TAG, "Socket bind failed");
        close(socket_fd);
        vTaskDelete(NULL);
        return;
    }


    ESP_LOGI(TAG, "DNS server ready");


    while (true)
    {
        struct sockaddr_in client;
        socklen_t client_length;
        client_length = sizeof(client);
        int request_length;

        request_length = recvfrom(socket_fd, request, sizeof(request), 0, (struct sockaddr *)&client, &client_length);

        if (request_length <= 0)
            continue;

        char domain[256];

        memset(domain, 0, sizeof(domain));


        if (!dns_get_domain( request, request_length, domain, sizeof(domain)))
            continue;

        ESP_LOGI(TAG, "Query %s", domain);


        if (blocklist_check(domain)){
            memcpy(response, request, request_length);
            dns_make_block_response(response);
            sendto(socket_fd, response, request_length, 0,(struct sockaddr *)&client, client_length);
	    block_counter_increment();
            continue;
        }


        int response_length;

        response_length = dns_forward(request, request_length, response);


        if (response_length > 0)
            sendto(socket_fd, response, response_length, 0, (struct sockaddr *)&client, client_length);
    }
}


esp_err_t dns_sinkhole_init(void)
{
    BaseType_t result;

    result = xTaskCreate(dns_task, "dns_task", 4096, NULL, 5, NULL);

    if (result != pdPASS)
        return ESP_FAIL;

    return ESP_OK;
}
