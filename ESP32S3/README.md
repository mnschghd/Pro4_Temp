# esp32s3-webdrive

> ESP32-S3 WiFi AP file server with drag-and-drop upload/download

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x+-orange)](https://github.com/espressif/esp-idf)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blueviolet)](https://platformio.org/)

A web-based file server running on the ESP32-S3. The device operates as a **WiFi access point** — connect to its network, open the dashboard in your browser, and manage files on the ESP's flash storage with drag-and-drop convenience.

---

## ✨ Features

- **Access Point Mode** — No router needed; the ESP32-S3 hosts its own WiFi network
- **File Browser** — Browse, open, and manage files on the device
- **Drag-and-Drop Upload** — Upload files by dragging them onto the browser window
- **Download** — Single-click file downloads
- **Directory Navigation** — Browse into subdirectories
- **Responsive UI** — Works on desktop and mobile browsers
- **LittleFS Storage** — Reliable flash filesystem for embedded use

## 🚀 Quick Start

### Prerequisites

| Requirement | Minimum Version |
|-------------|----------------|
| ESP32-S3 dev board | — |
| USB data cable | — |
| ESP-IDF or PlatformIO | ESP-IDF v5.x / PlatformIO Core 6+ |

### Build & Flash

```bash
# Clone the repo
git clone https://github.com/your-org/esp32s3-webdrive.git
cd esp32s3-webdrive

# Option A: ESP-IDF
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor

# Option B: PlatformIO
pio run --target upload
pio device monitor
```

Once flashed, the ESP32-S3 will broadcast a WiFi network. Connect to it, then open **http://192.168.4.1** in your browser.

⚠️ **No internet while connected.** The ESP hosts its own isolated WiFi network. Your device loses internet access when connected to the ESP's AP. This is by design — the ESP is a standalone file server, not a router.

## 📁 Project Structure

```
esp32s3-webdrive/
├── main/                  # Application source code
│   ├── CMakeLists.txt
│   ├── app_main.c         # Entry point
│   ├── wifi_ap.c/h        # WiFi access point logic
│   ├── http_server.c/h    # HTTP server & REST API
│   └── file_handler.c/h   # LittleFS file operations
├── frontend/              # Web UI source (HTML/CSS/JS)
│   ├── index.html
│   ├── style.css
│   └── app.js
├── docs/                  # Documentation
│   ├── HARDWARE.md
│   ├── SETUP.md
│   ├── API.md
│   └── DEVELOPMENT.md
├── partitions.csv         # Flash partition table
├── .gitignore
├── CHANGELOG.md
├── LICENSE
└── README.md
```

## 📚 Documentation

| File | Description |
|------|-------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System architecture & design decisions |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Board specifications & pinout |
| [docs/SETUP.md](docs/SETUP.md) | Development environment setup |
| [docs/API.md](docs/API.md) | HTTP API reference |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Coding standards & contributing |

## 🧪 Roadmap

- [x] WiFi AP mode
- [x] Basic HTTP server
- [x] File listing & navigation
- [ ] Drag-and-drop upload
- [ ] File download
- [ ] Directory creation
- [ ] File rename & delete
- [ ] mDNS / hostname resolution

## 📄 License

MIT — see [LICENSE](LICENSE).
