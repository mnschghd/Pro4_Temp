# Hardware

> ESP32-S3 specifications, pinout, and board recommendations for the file server.

---

## 1. ESP32-S3 Overview

| Specification | Value |
|---------------|-------|
| **CPU** | Dual-core Xtensa LX7 @ up to 240 MHz |
| **SRAM** | 512 KB |
| **PSRAM** | Up to 8 MB |
| **Flash** | 8–16 MB |
| **WiFi** | 802.11 b/g/n, 2.4 GHz, AP + STA modes |
| **Max AP clients** | 4 |

---

## 2. Recommended Boards

Any ESP32-S3 dev board with **8 MB flash or more** works.

| Board | Flash | PSRAM | Notes |
|-------|-------|-------|-------|
| **ESP32-S3-DevKitC-1** | 8 MB | 8 MB | Official Espressif board, widely available |
| **ESP32-S3-USB-OTG** | 16 MB | 8 MB | Extra USB-A port, useful for future expansion |
| **Generic S3 dev board** | 8 MB | — | Works if flash ≥ 8 MB |

---

## 3. Pinout (ESP32-S3-DevKitC-1)

```
┌──────────────────────┐
│  USB-UART (to PC)    │
└──────┬───────────────┘
       │
┌──────┴────┐
│ ESP32-S3  │
│           │
│ GPIO 48 → RGB LED    │
│ GPIO 0  → BOOT button│
│ GPIO 43 → UART TX    │
│ GPIO 44 → UART RX    │
└──┬──┬────┘
   │  │
┌──┴──┴──────┐
│  USB-C     │  (power + programming)
└────────────┘
```

| Pin | Function |
|-----|----------|
| GPIO 48 | RGB LED (status indication) |
| GPIO 0 | BOOT button (hold + reset for flash mode) |
| GPIO 43 | U0TXD (USB-UART TX) |
| GPIO 44 | U0RXD (USB-UART RX) |

---

## 4. Power

| Source | Voltage | Current |
|--------|---------|---------|
| USB (VBUS) | 5 V | ~200 mA (idle), ~400 mA (WiFi active) |
| External 3.3 V | 3.3 V | ~150–250 mA |

Use a **USB data cable** (not charge-only) for flashing and power.

---

## 5. Antenna

- **On-board PCB antenna** — default, adequate for short-range AP
- **External IPEX connector** — available on some boards for better range

---

*See [SETUP.md](SETUP.md) for dev environment setup and flashing.*
