#ifndef WEBDRIVE_CONFIG_H
#define WEBDRIVE_CONFIG_H

/**
 * @file webdrive_config.h
 * @brief User configuration for esp32s3-webdrive.
 *
 * Edit this file to change WiFi credentials and other settings.
 * After changing, rebuild and reflash:
 *   idf.py build && idf.py -p /dev/ttyACM0 flash
 */

// ============================================================================
// WiFi Access Point Settings
// ============================================================================

/** The WiFi network name (SSID). */
#define WEBDRIVE_WIFI_SSID       "ESP32S3-WebDrive"

/**
 * WiFi password (min 8 characters for WPA2).
 * The ESP will broadcast an open network if this is empty.
 */
#define WEBDRIVE_WIFI_PASSWORD   "esp32s3-webdrive"

/** WiFi channel (1–11). */
#define WEBDRIVE_WIFI_CHANNEL    1

/** Maximum number of connected clients. */
#define WEBDRIVE_WIFI_MAX_CLIENTS 4

// ============================================================================
// Storage
// ============================================================================

/** Maximum upload file size in bytes (default: 4 MB). */
#define WEBDRIVE_MAX_UPLOAD_SIZE (4 * 1024 * 1024)

#endif /* WEBDRIVE_CONFIG_H */
