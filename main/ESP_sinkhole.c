#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi.h"
#include "dns.h"
#include "web_ui.h"

void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "Starting");
    ESP_ERROR_CHECK(wifi_init());
    ESP_LOGI(TAG, "WiFi ready");
    ESP_ERROR_CHECK(dns_init());
    ESP_LOGI(TAG, "DNS ready");
    ESP_ERROR_CHECK(web_ui_init());
}
