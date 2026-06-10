# PRO4 Temp

## What's in here

**ESP32S3/** - firmware for an ESP32-S3 that turns into a WiFi file server.  
Flash it onto the board, connect to its network, and you get a browser-based file manager. Drag stuff onto the page, download files, browse around.
It uses the ESP-IDF framework, LittleFS on the flash, and a minimal HTTP server that serves a web UI from the chip itself. The frontend is plain HTML/CSS/JS, no frameworks.

**PI/** - a USB HID keylogger/passthrough setup for a Raspberry Pi 5.  
The idea: plug a keyboard into the Pi's USB-A port, plug the Pi into a PC via USB-C, and the Pi forwards every keystroke to the PC while also logging them. The setup script (`setup_hid_gadget.sh`) turns the Pi's USB-C into a virtual HID keyboard gadget. Then `keylogger.py` reads from the physical keyboard and writes to both `/dev/hidg0` and a log file.

## How to use

- For the ESP32-S3, open the `ESP32S3/` directory and flash with `idf.py` or PlatformIO. The default SSID and password are in `webdrive_config.h`.
- For the Pi, run `sudo ./PI/setup_hid_gadget.sh` once (it's idempotent), reboot if it says so, then `sudo ./PI/keylogger.py`.
