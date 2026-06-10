#pragma once
#include "esp_err.h"

/**
 * @brief Start the HTTP REST server.
 *
 * Registers all routes (static files + API) and begins listening on port 80.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t http_server_init(void);
