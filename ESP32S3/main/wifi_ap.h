#pragma once
#include "esp_err.h"

/**
 * @brief Initialize WiFi in soft-AP mode.
 *
 * SSID, password, and channel are configured via menuconfig
 * (WEBDRIVE_WIFI_SSID, WEBDRIVE_WIFI_PASSWORD, WEBDRIVE_WIFI_CHANNEL).
 * The AP serves IPs from 192.168.4.2–.10.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t wifi_ap_init(void);
