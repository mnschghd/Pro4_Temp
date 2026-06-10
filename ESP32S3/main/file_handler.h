#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include <stdio.h>

/**
 * @brief Mount SPIFFS and create the /spiffs base directory.
 *
 * Formats the partition automatically if mount fails.
 *
 * @return ESP_OK on success.
 */
esp_err_t fs_init(void);

/**
 * @brief Return total and used bytes on the SPIFFS partition.
 */
esp_err_t fs_stats(size_t *out_total, size_t *out_used);

/**
 * @brief Check whether a path exists and is a directory.
 */
bool fs_is_dir(const char *spiffs_path);

/**
 * @brief Check whether a path exists and is a regular file.
 */
bool fs_is_file(const char *spiffs_path);

/**
 * @struct fs_entry
 * @brief One entry returned by directory listing.
 */
typedef struct {
    char name[64];
    bool is_dir;
    size_t file_size;
} fs_entry_t;

/**
 * @brief List the contents of a directory.
 *
 * @param spiffs_path  Full path under /spiffs (e.g. "/" or "/subdir")
 * @param entries      Output array (caller must free with free())
 * @param count        Number of entries returned
 * @return ESP_OK, or ESP_FAIL if the directory cannot be opened.
 */
esp_err_t fs_list(const char *spiffs_path, fs_entry_t **entries, size_t *count);

/**
 * @brief Delete a file or empty directory.
 */
esp_err_t fs_delete(const char *spiffs_path);

/**
 * @brief Create a directory (and any intermediate parents).
 */
esp_err_t fs_mkdir(const char *spiffs_path);
