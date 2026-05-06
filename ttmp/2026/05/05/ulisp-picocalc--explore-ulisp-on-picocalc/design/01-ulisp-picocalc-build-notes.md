---
Title: uLisp PicoCalc Build Notes
Ticket: ulisp-picocalc
Status: active
Topics:
    - embedded
    - lisp
    - rp2040
    - picocalc
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/ulisp-picocalc.ino
    - /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/Setup60_RP2040_ILI9488.h
    - /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc-sketch/ulisp-picocalc-sketch.ino
ExternalSources: []
Summary: "Build instructions and notes for compiling uLisp for the PicoCalc using arduino-cli"
LastUpdated: 2026-05-05T19:05:00.000000000-04:00
WhatFor: "Reference for anyone wanting to compile uLisp for PicoCalc from source"
WhenToUse: "Follow these steps to reproduce the build or customize uLisp for PicoCalc"
---

# uLisp PicoCalc Build Notes

## Overview

uLisp is a Lisp interpreter for microcontrollers. The PicoCalc version turns the Clockwork Pi PicoCalc into a self-contained handheld Lisp computer with keyboard, 320x320 color display, and SD card.

Source: `technoblogy/ulisp-picocalc` on GitHub — this is the dedicated PicoCalc fork, no need to patch `ulisp-arm`.

## Hardware

- **MCU**: RP2040 (Raspberry Pi Pico) — dual-core ARM Cortex M0+ @ 125 MHz (overclockable to 200 MHz)
- **Display**: 320x320 ILI9488 IPS, driven via TFT_eSPI (SPI)
- **Keyboard**: Separate STM32 processor, communicates via I2C
- **Storage**: SD card slot + flash filesystem (LittleFS)
- **RAM**: 264KB (193KB used by uLisp = 73%)
- **Flash**: 2MB (split 1MB sketch / 1MB filesystem for save-image)

## Build Toolchain

### Prerequisites

```bash
# Install arduino-cli
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh

# Initialize and add RP2040 board URL
arduino-cli config init
arduino-cli config set board_manager.additional_urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

# Install RP2040 core (v5.6.0 tested)
arduino-cli core update-index
arduino-cli core install rp2040:rp2040

# Install TFT_eSPI v2.5.34
arduino-cli lib install TFT_eSPI@2.5.34

# Install PicoCalc keyboard library
cd ~/Arduino/libraries
git clone https://github.com/cuu/arduino_picocalc_kbd.git
```

### Patch TFT_eSPI

```bash
# Copy PicoCalc display setup
cp ulisp-picocalc/Setup60_RP2040_ILI9488.h \
   ~/Arduino/libraries/TFT_eSPI/User_Setups/

# Edit User_Setup_Select.h: comment out default, add PicoCalc setup
# Change:
#   #include <User_Setup.h>
# To:
#   // #include <User_Setup.h>
#   #include <User_Setups/Setup60_RP2040_ILI9488.h>
```

### Compile

```bash
arduino-cli compile \
    --fqbn rp2040:rp2040:rpipico \
    --build-path ./build \
    --warnings all \
    ./ulisp-picocalc-sketch
```

### Expected Warnings (harmless)

- `TOUCH_CS pin not defined` — PicoCalc has no touchscreen
- `variable 'result' might be clobbered by 'longjmp'` — known setjmp/longjmp pattern in uLisp

### Build Output

| File | Size | Notes |
|------|------|-------|
| `.ino.bin` | 230 KB | Raw binary |
| `.ino.uf2` | 460 KB | UF2 flash image (copy to Pico BOOTSEL drive) |
| Program memory | 211 KB (10%) | Of 2MB flash |
| Dynamic memory | 194 KB (73%) | Of 264KB RAM |

## Flashing

1. Hold BOOTSEL on the Pico, connect USB
2. Copy the `.uf2` file to the RPI-RP2 drive
3. Disconnect USB, power on PicoCalc

## Compatibility Notes

- **BIOS**: uLisp currently only works with BIOS 1.2 keyboard firmware. BIOS 1.4 has known incompatibilities.
- **Board**: Works with Pico (RP2040), Pico 2 (RP2350), and their Wi-Fi variants
- **Serial**: Default uses Serial1 (USB-C port). Comment out `#define Serial Serial1` to use Pico micro-USB instead.
- **SD Card**: Enabled by default. Comment out `#define sdcardsupport` to disable and save workspace to flash instead.

## Key uLisp PicoCalc Extensions

- `get-key` — wait for and return a keypress
- `read-pixel x y` — read pixel color from display
- `save-bmp filename` — save screenshot to SD card as BMP
- `note pin note octave` / `(note)` — play tones through speaker
- Autocomplete via Tab key
- ARM assembler for native code generation
- Graphics extensions (draw-pixel, draw-line, fill-screen, etc.)
