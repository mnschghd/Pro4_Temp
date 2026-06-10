#include "wifi_ap.h"
#include "webdrive_config.h"
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_ap";

esp_err_t wifi_ap_init(void)
{
    // ---- Init NVS (required by WiFi) ----
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ---- Init TCP/IP + default AP netif ----
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    // ---- Init WiFi with default config ----
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // ---- Configure soft-AP ----
    bool has_password = strlen(WEBDRIVE_WIFI_PASSWORD) > 0;

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WEBDRIVE_WIFI_SSID,
            .password = WEBDRIVE_WIFI_PASSWORD,
            .ssid_len = strlen(WEBDRIVE_WIFI_SSID),
            .max_connection = WEBDRIVE_WIFI_MAX_CLIENTS,
            .authmode = has_password ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN,
            .channel = WEBDRIVE_WIFI_CHANNEL,
        },
    };

    if (has_password && strlen(WEBDRIVE_WIFI_PASSWORD) < 8) {
        ESP_LOGW(TAG, "WPA2 password must be at least 8 characters — falling back to open AP");
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started — SSID: \"%s\"  channel: %d  auth: %s",
             WEBDRIVE_WIFI_SSID, WEBDRIVE_WIFI_CHANNEL,
             has_password ? "WPA2" : "OPEN");
    ESP_LOGI(TAG, "Connect and open http://192.168.4.1");

    return ESP_OK;
}
