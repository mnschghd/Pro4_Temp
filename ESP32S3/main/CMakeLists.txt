idf_component_register(
    SRCS
        app_main.c
        wifi_ap.c
        http_server.c
        file_handler.c
    INCLUDE_DIRS "."
    EMBED_FILES
        frontend/index.html
        frontend/style.css
        frontend/app.js
    PRIV_REQUIRES
        spiffs
        esp_http_server
        esp_wifi
        nvs_flash
        esp_timer
)
