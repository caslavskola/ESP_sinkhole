#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "http_api.h"
#include "blocklist.h"
#include "list_manager.h"
#include "storage.h"
#include "statistics.h"
#include "esp_http_server.h"
#define TAG "HTTP_API"

#define API_STATUS_URI          "/api/status"
#define API_BLOCKLIST_UPDATE_URI "/api/blocklist/update"
#define API_BLOCKLIST_ERASE_URI "/api/storage/erase/blocklist"

#define HTTP_RESPONSE_BUFFER_SIZE 256
#define HTTP_REQUEST_BUFFER_SIZE 256


static httpd_handle_t s_server = NULL;


static esp_err_t status_handler(
    httpd_req_t *request)
{
    char response[HTTP_RESPONSE_BUFFER_SIZE];

    uint64_t blocked = block_counter_get();
    size_t domains = blocklist_count();

    int length = snprintf(response, sizeof(response), "blocked:%llu\n domains:%lu\n", (unsigned long long)blocked, (unsigned long)domains);

    if (length < 0 || length >= sizeof(response))
        return httpd_resp_send_500(request);

    httpd_resp_set_type(request, "text/plain");

    return httpd_resp_send(request, response, length);
}


static esp_err_t blocklist_update_handler(
    httpd_req_t *request)
{
    char url[HTTP_REQUEST_BUFFER_SIZE];
    int received;
    esp_err_t result;

    if (request->content_len == 0
	|| request->content_len >= sizeof(url))
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid URL");
        return ESP_OK;
    }

    received = httpd_req_recv(request, url, request->content_len);

    if (received <= 0)
    {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Failed to read request");
        return ESP_OK;
    }

    url[received] = '\0';

    result = list_update(url);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Blocklist update failed: %s", esp_err_to_name(result));

        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Blocklist update failed");

        return ESP_OK;
    }

    httpd_resp_set_type(request, "text/plain");

    return httpd_resp_send(request, "{\"result\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}


static const httpd_uri_t status_uri =
{
    .uri = API_STATUS_URI,
    .method = HTTP_GET,
    .handler = status_handler,
    .user_ctx = NULL
};


static const httpd_uri_t blocklist_update_uri =
{
    .uri = API_BLOCKLIST_UPDATE_URI,
    .method = HTTP_POST,
    .handler = blocklist_update_handler,
    .user_ctx = NULL
};


static esp_err_t blocklist_erase_handler(
    httpd_req_t *request)
{
    storage_handle_t storage;
    esp_err_t result;

    result = storage_open(
        "blocklist",
        &storage);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to open blocklist storage: %s",
            esp_err_to_name(result));

        httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Failed to open blocklist storage");

        return ESP_OK;
    }

    result = storage_erase(&storage);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to erase blocklist: %s",
            esp_err_to_name(result));

        httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Failed to erase blocklist");

        return ESP_OK;
    }

    /*
     * The blocklist is no longer valid after erase.
     */
    blocklist_load();

    httpd_resp_set_type(
        request,
        "text/plain");

    return httpd_resp_send(
        request,
        "result=ok\n",
        HTTPD_RESP_USE_STRLEN);
}

static const httpd_uri_t blocklist_erase_uri =
{
    .uri = API_BLOCKLIST_ERASE_URI,
    .method = HTTP_POST,
    .handler = blocklist_erase_handler,
    .user_ctx = NULL
};

esp_err_t http_api_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t result;

    result = httpd_start(&s_server, &config);

    if (result != ESP_OK)
        return result;

    result = httpd_register_uri_handler(s_server, &status_uri);

    if (result != ESP_OK)
    {
        httpd_stop(s_server);
        s_server = NULL;
        return result;
    }

    result = httpd_register_uri_handler(
        s_server,
        &blocklist_update_uri);

    if (result != ESP_OK)
    {
        httpd_stop(s_server);
        s_server = NULL;
        return result;
    }

    result = httpd_register_uri_handler(
    s_server,
    &blocklist_erase_uri);

    if (result != ESP_OK){
        httpd_stop(s_server);
        s_server = NULL;
        return result;
    }

    ESP_LOGI(TAG, "HTTP API started");

    return ESP_OK;
}

