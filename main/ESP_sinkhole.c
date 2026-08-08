#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi.h"
#include "dns.h"
#include "statistics.h"
#include "http_api.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting");
    ESP_ERROR_CHECK(wifi_init());
    ESP_LOGI(TAG, "WiFi ready");
    ESP_ERROR_CHECK(dns_sinkhole_init());
    ESP_LOGI(TAG, "DNS ready");
    ESP_ERROR_CHECK(block_counter_init());
    ESP_LOGI(TAG, "Block counter ready");
    ESP_ERROR_CHECK(http_api_init());
    ESP_LOGI(TAG, "HTTP API ready");
}
