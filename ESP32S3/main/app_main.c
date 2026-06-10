#include <stdio.h>
#include "esp_log.h"

#include "wifi_ap.h"
#include "http_server.h"
#include "file_handler.h"

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32S3-WebDrive starting...");

    // 1. Mount filesystem
    ESP_ERROR_CHECK(fs_init());

    // 2. Start WiFi AP
    ESP_ERROR_CHECK(wifi_ap_init());

    // 3. Start HTTP server
    ESP_ERROR_CHECK(http_server_init());

    ESP_LOGI(TAG, "System ready — connect to WiFi and open http://192.168.4.1");
}
