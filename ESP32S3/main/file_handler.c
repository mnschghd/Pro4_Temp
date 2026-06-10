#include "file_handler.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "file_handler";
static const char *BASE = "/spiffs";

/* ---------- helpers ---------- */

static void make_full_path(const char *spiffs_path, char *buf, size_t bufsz)
{
    if (spiffs_path[0] == '/')
        snprintf(buf, bufsz, "%s%s", BASE, spiffs_path);
    else
        snprintf(buf, bufsz, "%s/%s", BASE, spiffs_path);
}

/* ---------- public API ---------- */

esp_err_t fs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BASE,
        .partition_label = "spiffs",
        .max_files = CONFIG_WEBDRIVE_SPIFFS_MAX_FILES,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    if (esp_spiffs_info("spiffs", &total, &used) == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted — %u KB total, %u KB used",
                 (unsigned)(total / 1024), (unsigned)(used / 1024));
    }
    return ESP_OK;
}

esp_err_t fs_stats(size_t *out_total, size_t *out_used)
{
    return esp_spiffs_info("spiffs", out_total, out_used);
}

bool fs_is_dir(const char *spiffs_path)
{
    char full[128];
    make_full_path(spiffs_path, full, sizeof(full));
    struct stat st;
    if (stat(full, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool fs_is_file(const char *spiffs_path)
{
    char full[128];
    make_full_path(spiffs_path, full, sizeof(full));
    struct stat st;
    if (stat(full, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

esp_err_t fs_list(const char *spiffs_path, fs_entry_t **out_entries, size_t *out_count)
{
    char full[128];
    make_full_path(spiffs_path, full, sizeof(full));

    DIR *dir = opendir(full);
    if (!dir) {
        ESP_LOGE(TAG, "Cannot open directory: %s", full);
        return ESP_FAIL;
    }

    // First pass: count entries
    size_t cap = 32;
    size_t n = 0;
    *out_entries = malloc(cap * sizeof(fs_entry_t));
    if (!*out_entries) {
        closedir(dir);
        return ESP_ERR_NO_MEM;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(ent->d_name, ".")  == 0) continue;
        if (strcmp(ent->d_name, "..") == 0) continue;

        if (n >= cap) {
            cap *= 2;
            fs_entry_t *tmp = realloc(*out_entries, cap * sizeof(fs_entry_t));
            if (!tmp) {
                free(*out_entries);
                *out_entries = NULL;
                closedir(dir);
                return ESP_ERR_NO_MEM;
            }
            *out_entries = tmp;
        }

        fs_entry_t *e = &(*out_entries)[n];
        strlcpy(e->name, ent->d_name, sizeof(e->name));

        if (ent->d_type == DT_DIR) {
            e->is_dir = true;
            e->file_size = 0;
        } else {
            e->is_dir = false;
            // stat to get real size
            char child[384];
            snprintf(child, sizeof(child), "%s/%s", full, ent->d_name);
            struct stat st;
            if (stat(child, &st) == 0)
                e->file_size = st.st_size;
            else
                e->file_size = 0;
        }
        n++;
    }
    closedir(dir);

    *out_count = n;
    return ESP_OK;
}

esp_err_t fs_delete(const char *spiffs_path)
{
    char full[128];
    make_full_path(spiffs_path, full, sizeof(full));

    struct stat st;
    if (stat(full, &st) != 0) {
        return ESP_ERR_NOT_FOUND;
    }

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(full) != 0) {
            ESP_LOGE(TAG, "rmdir failed (not empty?): %s", full);
            return ESP_FAIL;
        }
    } else {
        if (unlink(full) != 0) {
            ESP_LOGE(TAG, "unlink failed: %s", full);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t fs_mkdir(const char *spiffs_path)
{
    char full[128];
    make_full_path(spiffs_path, full, sizeof(full));

    // mkdir -p style: create parents as needed
    char *p = full + strlen(BASE) + 1; // skip "/spiffs/"
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(full, 0755);
            *p = '/';
        }
        p++;
    }
    if (mkdir(full, 0755) != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "mkdir failed: %s (errno=%d)", full, errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}
