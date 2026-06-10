# Development Guide

> Coding standards and workflow for esp32s3-webdrive.

---

## 1. Branch Strategy

```
main     ─── Production releases
dev      ─── Integration branch
feat/*   ─── Feature branches
fix/*    ─── Bugfix branches
docs/*   ─── Documentation
```

---

## 2. Commit Style

[Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add drag-and-drop upload endpoint
fix: correct Content-Type for file downloads
docs: add API reference for /api/status
refactor: extract HTTP route table into separate module
```

---

## 3. Code Standards

### C / C++

```c
// Naming
#define CONFIG_WIFI_SSID "ESP32S3-WebDrive"  // MACRO_CASE
typedef enum {                                // Upper snake
    FS_OK,
    FS_ERROR_NOT_FOUND
} fs_error_t;                                 // _t suffix

// Functions: snake_case
esp_err_t wifi_ap_start(void);

// File-scope globals: s_ prefix
static httpd_handle_t s_server = NULL;
```

**Guidelines:**
- K&R brace style
- Check all ESP-IDF return values
- Functions ≤ 50 lines
- One statement per line

### JavaScript (Frontend)

```javascript
const API_BASE = '/api/files';     // UPPER_CASE constants
let currentPath = '/';             // camelCase
function loadDirectory(path) { }   // camelCase
class FileBrowser { }              // PascalCase
```

---

## 4. Module Layout

| Module | Files | Responsibility |
|--------|-------|---------------|
| **WiFi AP** | `wifi_ap.c/h` | Initialize and manage softAP |
| **HTTP Server** | `http_server.c/h` | REST API routes and handlers |
| **File Handler** | `file_handler.c/h` | LittleFS read/write/delete |

---

## 5. Testing Checklist

Before merging to `dev`:

- [ ] Flashes successfully on clean ESP32-S3
- [ ] AP broadcasts and accepts clients
- [ ] File listing works at root and subdirectories
- [ ] Upload a small file succeeds
- [ ] Upload a large file returns proper error if too big
- [ ] Download returns correct content and headers
- [ ] Delete removes the file
- [ ] Create directory appears in listing
- [ ] Path traversal (`../`) is rejected
- [ ] Free heap stable after 100 operations

---

## 6. Debugging

```c
ESP_LOGI(TAG, "File uploaded: %s (%d bytes)", path, size);
ESP_LOGE(TAG, "FS error: %s", error_str);
ESP_LOGD(TAG, "Free heap: %d", esp_get_free_heap_size());
```

Enable verbose logging in menuconfig:
```
Component config → Log output → Default log verbosity → Debug
```

---

## 7. CI/CD

```yaml
# .github/workflows/build.yml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install ESP-IDF
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.3
          target: esp32s3
      - name: Build
        run: idf.py build
```

---

*For API details, see [API.md](API.md). For hardware specs, see [HARDWARE.md](HARDWARE.md).*
