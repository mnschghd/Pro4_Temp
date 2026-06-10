# Setup Guide

> Development environment setup for esp32s3-webdrive.

---

## Prerequisites

| Tool | Purpose |
|------|---------|
| **Git** | Version control |
| **Python 3.8+** | ESP-IDF toolchain |
| **CMake & Ninja** | Build system (ESP-IDF) |

---

## Option A: ESP-IDF (Native)

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh esp32s3
```

Add to `~/.bashrc`:

```bash
alias get_idf='. $HOME/esp/esp-idf/export.sh'
```

```bash
get_idf
idf.py --version
```

---

## Option B: PlatformIO

```bash
pip install platformio
pio --version
```

---

## Build & Flash

### ESP-IDF

```bash
cd esp32s3-webdrive
idf.py set-target esp32s3
idf.py menuconfig
idf.py -p /dev/ttyACM0 flash monitor
```

### PlatformIO

```bash
pio run --target upload
pio device monitor
```

### Finding the Port

| OS | Port |
|----|------|
| Linux | `/dev/ttyACM0` or `/dev/ttyUSB0` |
| macOS | `/dev/cu.usbserial-*` |
| Windows | `COM3` |

---

## First Boot

1. Flash the firmware
2. Monitor should show:

   ```
   I (0) app_main: ESP32S3-WebDrive v1.0.0
   I (100) wifi_ap: AP started, IP: 192.168.4.1
   I (200) http_server: HTTP server on port 80
   I (300) littlefs: Mounted, free: 1.5 MB
   ```

3. Connect to WiFi `ESP32S3-WebDrive`
4. Open **http://192.168.4.1**

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Flash fails to connect | Hold BOOT + tap RST, release BOOT |
| No WiFi found | Check `wifi_ap: AP started` in logs |
| Permission denied | `sudo usermod -aG dialout $USER` + re-login |

---

## Quick Reference

```bash
get_idf
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```
