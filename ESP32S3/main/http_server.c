#include "http_server.h"
#include "file_handler.h"
#include "webdrive_config.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

static const char *TAG = "http_server";

/* ---- embedded frontend symbols (from EMBED_FILES) ---- */
extern const char _binary_index_html_start[];
extern const char _binary_index_html_end[];
extern const char _binary_style_css_start[];
extern const char _binary_style_css_end[];
extern const char _binary_app_js_start[];
extern const char _binary_app_js_end[];

/* ---------- helpers ---------- */

static const char *mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcasecmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcasecmp(ext, ".css")  == 0) return "text/css; charset=utf-8";
    if (strcasecmp(ext, ".js")   == 0) return "application/javascript";
    if (strcasecmp(ext, ".json") == 0) return "application/json";
    if (strcasecmp(ext, ".png")  == 0) return "image/png";
    if (strcasecmp(ext, ".jpg")  == 0 || strcasecmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, ".txt")  == 0) return "text/plain; charset=utf-8";
    if (strcasecmp(ext, ".pdf")  == 0) return "application/pdf";
    if (strcasecmp(ext, ".zip")  == 0) return "application/zip";
    return "application/octet-stream";
}

/** Strip a URI prefix and return the sub-path. */
static const char *subpath(const char *uri, const char *prefix)
{
    size_t plen = strlen(prefix);
    if (strncmp(uri, prefix, plen) == 0) {
        const char *rest = uri + plen;
        return (*rest == '\0') ? "/" : rest;
    }
    return uri;
}

/** Reject path-traversal attempts. */
static bool path_safe(const char *path)
{
    return strstr(path, "..") == NULL;
}

/* ---------- frontend handlers ---------- */

static esp_err_t get_index(httpd_req_t *req)
{
    size_t len = _binary_index_html_end - _binary_index_html_start;
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, _binary_index_html_start, len);
    return ESP_OK;
}

static esp_err_t get_style(httpd_req_t *req)
{
    size_t len = _binary_style_css_end - _binary_style_css_start;
    httpd_resp_set_type(req, "text/css; charset=utf-8");
    httpd_resp_send(req, _binary_style_css_start, len);
    return ESP_OK;
}

static esp_err_t get_app_js(httpd_req_t *req)
{
    size_t len = _binary_app_js_end - _binary_app_js_start;
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, _binary_app_js_start, len);
    return ESP_OK;
}

/* ---------- GET /api/files — list directory ---------- */

static esp_err_t api_files_get(httpd_req_t *req)
{
    // Determine sub-path
    const char *spath;
    if (strcmp(req->uri, "/api/files") == 0) {
        spath = "/";
    } else {
        spath = subpath(req->uri, "/api/files");
    }

    // Check if this is a download request (?download)
    char query[64];
    bool want_download = false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[16];
        if (httpd_query_key_value(query, "download", val, sizeof(val)) == ESP_OK) {
            want_download = true;
        }
    }

    if (want_download && fs_is_file(spath)) {
        // --- Stream file for download ---
        char full[128];
        snprintf(full, sizeof(full), "/spiffs%s", spath);

        FILE *f = fopen(full, "rb");
        if (!f) {
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_sendstr(req, "{\"error\":\"Cannot open file\"}");
            return ESP_OK;
        }

        // Figure out filename for Content-Disposition
        const char *fname = strrchr(spath, '/');
        fname = fname ? fname + 1 : spath + 1;

        httpd_resp_set_type(req, mime_type(spath));
        char disp[128];
        snprintf(disp, sizeof(disp), "attachment; filename=\"%s\"", fname);
        httpd_resp_set_hdr(req, "Content-Disposition", disp);

        // Stream in chunks
        char buf[512];
        size_t nread;
        while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
            if (httpd_resp_send_chunk(req, buf, nread) != ESP_OK) {
                ESP_LOGW(TAG, "Download aborted by client");
                break;
            }
        }
        fclose(f);
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

    // --- List directory ---
    if (!fs_is_dir(spath)) {
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_sendstr(req, "{\"error\":\"Path not found\"}");
        return ESP_OK;
    }

    fs_entry_t *entries = NULL;
    size_t count = 0;
    if (fs_list(spath, &entries, &count) != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Failed to list directory\"}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"path\":\"");
    httpd_resp_sendstr_chunk(req, spath);
    httpd_resp_sendstr_chunk(req, "\",\"entries\":[");

    for (size_t i = 0; i < count; i++) {
        if (i > 0) httpd_resp_sendstr_chunk(req, ",");

        char buf[256];
        if (entries[i].is_dir) {
            snprintf(buf, sizeof(buf),
                     "{\"name\":\"%s\",\"type\":\"directory\"}",
                     entries[i].name);
        } else {
            snprintf(buf, sizeof(buf),
                     "{\"name\":\"%s\",\"type\":\"file\",\"size\":%u}",
                     entries[i].name, (unsigned)entries[i].file_size);
        }
        httpd_resp_sendstr_chunk(req, buf);
    }
    free(entries);

    httpd_resp_sendstr_chunk(req, "]}");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/* ---------- POST /api/upload — upload file ---------- */

static esp_err_t api_upload_post(httpd_req_t *req)
{
    const char *spath = subpath(req->uri, "/api/upload");

    if (!path_safe(spath)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\":\"Path traversal denied\"}");
        return ESP_OK;
    }

    if (strlen(spath) <= 1) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\":\"Missing filename\"}");
        return ESP_OK;
    }

    char full[128];
    snprintf(full, sizeof(full), "/spiffs%s", spath);

    // Ensure parent directory exists
    char dir[128];
    strlcpy(dir, full, sizeof(dir));
    char *slash = strrchr(dir, '/');
    if (slash && slash != dir) {
        *slash = '\0';
        mkdir(dir, 0755);  // best-effort
    }

    FILE *f = fopen(full, "wb");
    if (!f) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Cannot create file\"}");
        return ESP_OK;
    }

    char buf[512];
    int received, total = 0;
    int max_size = WEBDRIVE_MAX_UPLOAD_SIZE;

    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, received, f);
        total += received;

        if (total > max_size) {
            fclose(f);
            unlink(full);
            httpd_resp_set_status(req, "413 Payload Too Large");
            httpd_resp_sendstr(req, "{\"error\":\"File too large\"}");
            return ESP_OK;
        }
    }
    fclose(f);

    ESP_LOGI(TAG, "Uploaded: %s (%d bytes)", spath, total);

    char resp[192];
    snprintf(resp, sizeof(resp),
             "{\"status\":\"ok\",\"path\":\"%s\",\"size\":%d}", spath, total);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "201 Created");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/* ---------- DELETE /api/files — delete file/dir ---------- */

static esp_err_t api_files_del(httpd_req_t *req)
{
    const char *spath = subpath(req->uri, "/api/files");

    if (!path_safe(spath)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\":\"Path traversal denied\"}");
        return ESP_OK;
    }

    esp_err_t ret = fs_delete(spath);
    if (ret == ESP_ERR_NOT_FOUND) {
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_sendstr(req, "{\"error\":\"Not found\"}");
        return ESP_OK;
    }
    if (ret != ESP_OK) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_sendstr(req, "{\"error\":\"Could not delete (dir not empty?)\"}");
        return ESP_OK;
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"status\":\"deleted\",\"path\":\"%s\"}", spath);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/* ---------- POST /api/dir — create directory ---------- */

static esp_err_t api_dir_post(httpd_req_t *req)
{
    const char *spath = subpath(req->uri, "/api/dir");

    if (!path_safe(spath)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"error\":\"Path traversal denied\"}");
        return ESP_OK;
    }

    if (fs_mkdir(spath) != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Failed to create directory\"}");
        return ESP_OK;
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"status\":\"created\",\"path\":\"%s\"}", spath);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "201 Created");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/* ---------- GET /api/status ---------- */

static esp_err_t api_status_get(httpd_req_t *req)
{
    size_t total = 0, used = 0;
    fs_stats(&total, &used);

    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    unsigned long uptime = esp_timer_get_time() / 1000000ULL;

    char resp[512];
    snprintf(resp, sizeof(resp),
             "{"
             "\"device\":\"ESP32-S3\","
             "\"uptime\":%lu,"
             "\"storage\":{"
             "\"total\":%u,\"used\":%u,\"free\":%u,"
             "\"filesystem\":\"spiffs\""
             "},"
             "\"memory\":{"
             "\"free_heap\":%lu,\"psram_size\":%lu,\"free_psram\":%lu"
             "}"
             "}",
             uptime,
             (unsigned)total, (unsigned)used, (unsigned)(total - used),
             (unsigned long)free_heap,
             (unsigned long)psram_size,
             (unsigned long)free_psram);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/* ---------- server start ---------- */

esp_err_t http_server_init(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 16;
    cfg.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    /* ---- frontend routes ---- */
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/", .method = HTTP_GET, .handler = get_index });
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/index.html", .method = HTTP_GET, .handler = get_index });
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/style.css", .method = HTTP_GET, .handler = get_style });
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/app.js", .method = HTTP_GET, .handler = get_app_js });

    /* ---- API routes ---- */
    // GET /api/files
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/files", .method = HTTP_GET, .handler = api_files_get });
    // GET /api/files/*
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/files/*", .method = HTTP_GET, .handler = api_files_get });
    // DELETE /api/files/*
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/files/*", .method = HTTP_DELETE, .handler = api_files_del });
    // POST /api/dir/*
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/dir/*", .method = HTTP_POST, .handler = api_dir_post });
    // POST /api/upload/*
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/upload/*", .method = HTTP_POST, .handler = api_upload_post });
    // GET /api/status
    httpd_register_uri_handler(server, &(httpd_uri_t){
        .uri = "/api/status", .method = HTTP_GET, .handler = api_status_get });

    ESP_LOGI(TAG, "HTTP server listening on port %d", cfg.server_port);
    return ESP_OK;
}
