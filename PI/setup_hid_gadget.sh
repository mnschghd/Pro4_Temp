#!/bin/bash
# setup_hid_gadget.sh
#
# One-shot setup: turn the Pi 5's USB-C port into a USB HID keyboard.
# After running this and rebooting (if prompted), /dev/hidg0 will appear
# and writing 8-byte boot-keyboard reports to it sends keystrokes to
# whatever PC the USB-C cable is plugged into.
#
# Usage:   sudo ./setup_hid_gadget.sh
# Run once. Idempotent.

set -e

if [[ $EUID -ne 0 ]]; then
    echo "Run as root: sudo $0" >&2
    exit 1
fi

CONFIG_TXT=/boot/firmware/config.txt
NEEDS_REBOOT=0

# 1. dwc2 overlay in peripheral mode (Pi 5 USB-C)
if ! grep -q '^dtoverlay=dwc2' "$CONFIG_TXT"; then
    echo "Adding dtoverlay=dwc2 to $CONFIG_TXT"
    echo 'dtoverlay=dwc2,dr_mode=peripheral' >> "$CONFIG_TXT"
    NEEDS_REBOOT=1
fi

# 2. libcomposite at boot
if ! grep -q '^libcomposite' /etc/modules; then
    echo "Adding libcomposite to /etc/modules"
    echo 'libcomposite' >> /etc/modules
    NEEDS_REBOOT=1
fi

# 3. The gadget-creation script (runs every boot)
cat > /usr/local/sbin/hid-gadget-setup <<'GADGET'
#!/bin/bash
set -e
modprobe libcomposite
mount -t configfs none /sys/kernel/config 2>/dev/null || true
cd /sys/kernel/config/usb_gadget/
[ -d pi_keyboard ] && [ -s pi_keyboard/UDC ] && exit 0
[ -d pi_keyboard ] && rm -rf pi_keyboard

mkdir -p pi_keyboard
cd pi_keyboard

# Generic IDs. Change to mimic a specific keyboard if needed.
echo 0x1d6b > idVendor      # Linux Foundation
echo 0x0104 > idProduct     # Multifunction composite gadget
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB        # USB 2.0

mkdir -p strings/0x409
echo "0000000000"   > strings/0x409/serialnumber
echo "PRO3"         > strings/0x409/manufacturer
echo "USB Keyboard" > strings/0x409/product

mkdir -p configs/c.1/strings/0x409
echo "Config 1: Keyboard" > configs/c.1/strings/0x409/configuration
echo 250 > configs/c.1/MaxPower

# Boot-protocol HID keyboard function
mkdir -p functions/hid.usb0
echo 1 > functions/hid.usb0/protocol      # 1 = keyboard
echo 1 > functions/hid.usb0/subclass      # 1 = boot interface
echo 8 > functions/hid.usb0/report_length

# Standard boot keyboard report descriptor (8-byte report: mods, resv, 6 keys)
printf '\x05\x01\x09\x06\xa1\x01\x05\x07\x19\xe0\x29\xe7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x03\x95\x05\x75\x01\x05\x08\x19\x01\x29\x05\x91\x02\x95\x01\x75\x03\x91\x03\x95\x06\x75\x08\x15\x00\x25\x65\x05\x07\x19\x00\x29\x65\x81\x00\xc0' \
    > functions/hid.usb0/report_desc

ln -s functions/hid.usb0 configs/c.1/

# Bind to the first available UDC (the USB-C port)
ls /sys/class/udc | head -n 1 > UDC
GADGET
chmod +x /usr/local/sbin/hid-gadget-setup

# 4. systemd unit so it runs at boot
cat > /etc/systemd/system/hid-gadget.service <<'UNIT'
[Unit]
Description=USB HID keyboard gadget
After=local-fs.target systemd-modules-load.service
RequiresMountsFor=/sys/kernel/config

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/hid-gadget-setup
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
UNIT

systemctl daemon-reload
systemctl enable hid-gadget.service

# 5. python-evdev for the keylogger script
if ! dpkg -s python3-evdev >/dev/null 2>&1; then
    echo "Installing python3-evdev..."
    apt-get update && apt-get install -y python3-evdev
fi

echo
if [[ $NEEDS_REBOOT -eq 1 ]]; then
    echo "Setup complete. Reboot required: sudo reboot"
    echo "After reboot, /dev/hidg0 should exist."
else
    echo "Setup complete. Starting gadget now..."
    systemctl start hid-gadget.service
    echo "Check: ls -l /dev/hidg0"
fi
