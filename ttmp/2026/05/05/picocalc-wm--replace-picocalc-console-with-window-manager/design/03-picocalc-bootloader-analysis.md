---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: PicoCalc/Code/pico_multi_booter/linker_scripts/memmap_default.ld.mp.rp2040
      Note: ClockworkPi linker script for 200 KiB offset BIN apps
    - Path: PicoCalc/Code/pico_multi_booter/sd_boot/main.c
      Note: Stock/legacy ClockworkPi multibooter source; shows SD_BOOT_FLASH_OFFSET and BIN flashing logic
    - Path: uf2loader/README.md
      Note: UF2 Loader installation and usage instructions
    - Path: uf2loader/stage3/stage3.c
      Note: UF2 Loader boot-stage logic and key shortcuts
    - Path: uf2loader/stage3/uf2.c
      Note: UF2 Loader UF2 parser/loader
    - Path: uf2loader/ui/main.c
      Note: UF2 Loader menu UI entry point
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# PicoCalc Bootloader v1.0 vs UF2 Loader Analysis

## Goal

Explain why ordinary `.uf2 -> .bin` conversion did not boot uLisp from the stock PicoCalc bootloader, what the stock bootloader expects, and why `pelrun/uf2loader` is the better development path for uLisp experimentation.

## The important distinction

There are two different bootloader families in play:

1. **ClockworkPi stock / legacy `pico_multi_booter` v1.0-style bootloader**
   - Shows `/firmware/*.bin` files.
   - Copies the selected `.bin` into flash at `SD_BOOT_FLASH_OFFSET`.
   - The app must be linked to run at that offset.
   - A normal UF2 converted to BIN is not enough.

2. **`pelrun/uf2loader`**
   - Uses ordinary `.uf2` app files.
   - Has a SD-card UI/menu loaded from `BOOT2040.UF2`.
   - Avoids per-app custom linker scripts for normal apps.
   - Supports USB mass storage for SD access after entering the UI.

For rapid uLisp development, `uf2loader` is the safer and more convenient option.

## Why the converted BINs failed

The BINs we extracted from normal UF2 files begin with the RP2040 boot2 code, not a vector table:

```text
uLisp_4.8f.bin header:
  w0 = 0x4b32b500
  w1 = 0x60582021
  w2 = 0x21026898
  w3 = 0x60984388
```

A valid legacy multibooter BIN begins with a vector table:

```text
Lua_180a58e.bin header:
  SP    = 0x20042000
  Reset = 0x100320f7
  NMI   = 0x100320c3
  HardF = 0x100320c5
```

The stock bootloader writes the BIN into flash at the app offset and then validates/jumps using the vector table there. Therefore the first word must be a plausible SRAM stack pointer (`0x20000000..0x20042000`) and the second word must be a reset vector inside app flash (`0x10032000..`). A normal UF2-derived BIN has neither.

## What the legacy bootloader does

The ClockworkPi `pico_multi_booter` source defines:

```c
#define SD_BOOT_FLASH_OFFSET (200 * 1024)
```

It loads a selected `.bin` like this:

```c
flash_range_erase(SD_BOOT_FLASH_OFFSET + program_size, FLASH_SECTOR_SIZE);
flash_range_program(SD_BOOT_FLASH_OFFSET + program_size, buffer, len);
```

Then it considers the installed app valid if:

```c
stack_pointer >= 0x20000000 && stack_pointer <= MAX_RAM + 8KB
reset_vector >= 0x10000000 + SD_BOOT_FLASH_OFFSET
reset_vector <= 0x10000000 + PICO_FLASH_SIZE_BYTES
```

That means a legacy-compatible `.bin` must be linked with `FLASH` origin at `0x10000000 + 200k` and must start at `.text`/vectors, not boot2.

ClockworkPi ships an Arduino-Pico linker script template for this:

```ld
MEMORY
{
    FLASH(rx) : ORIGIN = 0x10000000 + 200k, LENGTH = __FLASH_LENGTH__ - 200k
    RAM(rwx) : ORIGIN = 0x20000000, LENGTH = __RAM_LENGTH__
}

.text : {
    __logical_binary_start = .;
    KEEP (*(.vectors))
    KEEP (*(.binary_info_header))
    KEEP (*(.reset))
    ...
} > FLASH
```

The key difference from the normal Arduino-Pico linker script is that the multiboot app image starts with `.vectors`, not `.boot2`.

## What `uf2loader` does differently

`uf2loader` splits booting into two pieces:

- A small flashed bootloader (`bootloader_pico.uf2`) that occupies protected high flash.
- A menu UI loaded from SD (`BOOT2040.UF2`) into RAM when requested.

Its README says to install:

```text
SD root:
  BOOT2040.UF2
  pico1-apps/*.uf2

Flash once via BOOTSEL:
  bootloader_pico.uf2
```

Usage:

- Normal power-on boots the currently installed app.
- Hold **Up**, **F1**, or **F5** during power-on to enter the loader menu.
- Hold **Down** or **F3** during power-on to enter BOOTSEL.
- If the menu cannot be loaded, it falls back to BOOTSEL.

The UI source (`uf2loader/ui/main.c`) initializes the LCD, mounts the SD card, draws the directory UI, and accepts only `.uf2` selections:

```c
lcd_init();
fs_init();
text_directory_ui_init();
keypad_init();
text_directory_ui_run();
```

When a UF2 is selected, it calls:

```c
load_application_from_uf2(path);
reboot();
```

The stage3 boot code then boots either the installed app or loads the menu from `BOOT2040.UF2`.

## What was installed on the SD card

I installed these files onto the SD card:

```text
/BOOT2040.UF2
/pico1-apps/uLisp_4.8f_arduino_pico_4.5.0.uf2
/firmware/uf2loader_bootloader_pico_v2.4.1.uf2
/firmware/uf2loader_diag_pico_v2.4.1.uf2
```

The SD card was then synced and unmounted cleanly.

## Recommendation

For your current goal — repeatedly testing uLisp firmware while exploring window-manager work — use `uf2loader`.

Reasons:

- You can copy normal Arduino-generated `.uf2` files into `pico1-apps/`.
- You do not need to solve the legacy `.bin` linker problem for every build.
- The menu is graphical/text-based on the PicoCalc LCD.
- It has a clear recovery path: hold Down/F3 for BOOTSEL.
- It supports USB mass storage access to the SD card after entering the UI.

Keep the legacy bootloader notes because they explain why `/firmware/*.bin` did not work, but do not spend more time fighting that path unless there is a specific reason to keep the stock v1.0 bootloader.

## How to flash `uf2loader` itself

You cannot install `bootloader_pico.uf2` merely by placing it in `/firmware`; the stock bootloader expects offset-linked BIN apps there. To replace the bootloader, flash it via RP2040 BOOTSEL:

1. Power off the PicoCalc.
2. Enter BOOTSEL mode for the Pico module:
   - Hold the Pico BOOTSEL button while connecting USB to the Pico, or
   - Use any existing firmware/bootloader feature that drops into BOOTSEL if available.
3. A USB drive named `RPI-RP2` appears on your computer.
4. Copy `bootloader_pico.uf2` to `RPI-RP2`.
5. The Pico reboots automatically.
6. Insert the SD card containing `BOOT2040.UF2` and `pico1-apps/`.
7. Hold **Up/F1/F5** on power-on to enter the UF2 loader menu.

If you want a quick diagnostic before flashing the bootloader, flash `diag_pico.uf2` via BOOTSEL instead. It checks SD mounting and whether `BOOT2040.UF2` is readable.
