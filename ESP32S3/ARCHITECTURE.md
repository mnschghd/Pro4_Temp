# Architecture

> System design for the esp32s3-webdrive file server.

---

## 1. System Overview

The ESP32-S3 runs a lightweight HTTP file server on its WiFi access point.

```
┌─────────────────────────────────────────────────┐
│                  Browser                         │
│  (connect to ESP's WiFi, open 192.168.4.1)      │
└────────────────────┬────────────────────────────┘
                     │ HTTP (REST)
                     ▼
┌─────────────────────────────────────────────────┐
│              ESP32-S3 (AP Mode)                  │
│                                                  │
│  ┌──────────────┐   ┌───────────────────────┐   │
│  │  WiFi AP     │   │   HTTP Server          │   │
│  │  (softAP)    │──▶│   (esp_http_server)    │   │
│  │              │   │                        │   │
│  │  IP: 192.168 │   │  GET    /           │   │   │
│  │  .4.1        │   │  GET    /api/files   │   │   │
│  └──────────────┘   │  POST   /api/upload   │   │   │
│                      │  DELETE /api/files/*  │   │   │
│                      └───────────┬───────────┘   │
│                                  │               │
│                      ┌───────────▼───────────┐   │
│                      │  File System          │   │
│                      │  (LittleFS)           │   │
│                      │                       │   │
│                      │  /index.html          │   │
│                      │  /style.css           │   │
│                      │  /app.js              │   │
│                      │  /uploads/            │   │
│                      └───────────────────────┘   │
└─────────────────────────────────────────────────┘
```

---

## 2. WiFi Access Point

The ESP32-S3 operates as a **soft-AP** (software-enabled access point).

| Parameter | Value |
|-----------|-------|
| SSID | `ESP32S3-WebDrive` (configurable) |
| Password | configurable |
| Max connections | 4 |
| IP address | `192.168.4.1` |
| DHCP pool | `192.168.4.2` – `192.168.4.10` |

---

## 3. HTTP Server

Built on **esp_http_server** (ESP-IDF) or **ESPAsyncWebServer** (Arduino).

| Route | Method | Description |
|-------|--------|-------------|
| `/` | GET | Serve the web UI (`index.html`) |
| `/api/files` | GET | List root directory |
| `/api/files/*` | GET | List subdirectory or download file (with `?download`) |
| `/api/upload/*` | POST | Upload a file |
| `/api/files/*` | DELETE | Delete a file or empty directory |
| `/api/dir/*` | POST | Create a directory |
| `/api/status` | GET | Device info (free heap, flash usage, uptime) |

> Full reference: [docs/API.md](docs/API.md)

---

## 4. Filesystem

### 4.1 Storage

| Aspect | Choice |
|--------|--------|
| **Filesystem** | LittleFS |
| **Partition size** | 2–8 MB (configurable) |
| **Max file path** | 64 characters |

### 4.2 Partition Table

```
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0x14000, 0x2000
app0,     app,  ota_0,   0x10000, 0x1E0000
app1,     app,  ota_1,   0x200000,0x1E0000
littlefs, data, spiffs,  0x3E0000,0x200000  # 2 MB storage
```

---

## 5. Frontend

Single-page application served from flash. No external dependencies.

```
index.html
  └── app.js
        ├── FileBrowser       → renders directory listing
        ├── DragDropUploader  → handles drag-and-drop events
        ├── DownloadHandler   → triggers file downloads
        └── API client        → wraps fetch() for all endpoints
```

---

## 6. Data Flow: Upload

```
Browser                          ESP32-S3
   │                                │
   │  User drops file.zip           │
   │────────────────────────────────▶│
   │  POST /api/upload/file.zip     │
   │  Content-Type: octet-stream    │
   │────────────────────────────────▶│
   │                                │
   │         http_server recvs chunk │
   │         file_handler writes to   │
   │         LittleFS                 │
   │         repeat until complete    │
   │                                │
   │  201 Created { "size": 12345 }  │
   │◀────────────────────────────────│
   │                                │
   │  GET /api/files (refresh)      │
   │────────────────────────────────▶│
   │  [{ name: "file.zip", ... }]   │
   │◀────────────────────────────────│
```

---

## 7. Design Decisions

| # | Decision | Rationale |
|---|----------|-----------|
| 1 | **LittleFS over SPIFFS** | Better performance, no file size cap, proper wear-leveling |
| 2 | **REST API over WebSockets** | Simpler implementation, universal browser support, lower memory |
| 3 | **SPA frontend** | No page reloads, single-file JS bundle fits in flash |
| 4 | **AP mode only** | Keeps firmware simple; STA can be added later |
| 5 | **Embedded UI in flash** | Zero external dependencies, works fully offline |

---

## 8. Memory Budget

| Component | Approx. RAM |
|-----------|-------------|
| WiFi soft-AP | 10 KB |
| HTTP server | 15 KB |
| File operations buffer | 4 KB |
| Upload chunk buffer | 4 KB |
| **Total reserved** | **~33 KB** |
| **Free (PSRAM)** | **~8 MB** |

---

*See [docs/API.md](docs/API.md) for endpoint details and [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for implementation.*
