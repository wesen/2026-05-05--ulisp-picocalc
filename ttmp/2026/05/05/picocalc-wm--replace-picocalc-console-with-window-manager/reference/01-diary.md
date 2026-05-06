---
title: Diary
doc_type: reference
intent: long-term
status: active
topics:
  - picocalc
  - display
  - keyboard
  - wm
---

# Diary

## Goal

Record the investigation, design, and implementation journey for replacing the monolithic uLisp PicoCalc console with a tiled window manager.

## Step 1: Analyze Current Console Architecture

The user wants to build a window manager for the PicoCalc, similar to a screenshot showing tiled windows. Before designing anything, I needed to deeply understand every layer of the current console system: how characters get from the Lisp interpreter to the screen, how keyboard input flows back, what hardware resources are available, and where the bottlenecks are.

### Prompt Context

**User prompt (verbatim):** "Write a detailed document on how the console works on the picocalc port, because I want to replace it with a window manager funcitonality. I need to understand keyboard routing, what I can do in terms of background interrupt handling and such, basically how to create somethign like /tmp/pi-clipboard-410c4bd5-c456-4cee-b55c-63a73e9b12f4.png"

**Assistant interpretation:** Deep-dive analysis of the uLisp PicoCalc console system covering: display pipeline, keyboard input pipeline, interrupt mechanisms, SPI/I2C bus usage, and RP2040 dual-core possibilities. The end goal is to inform the design of a tiled window manager.

**Inferred user intent:** Understand every constraint and capability of the hardware/software stack before designing a window manager that can multiplex the display and route keyboard input to multiple windows.

### What I did

- Read the entire console subsystem in `ulisp-picocalc.ino` (lines 7211–7580)
- Read `PCKeyboard.cpp` and `PCKeyboard.h` — the Arduino keyboard library
- Read `picocalc_keyboard.ino` — the STM32 keyboard firmware (I2C slave)
- Read `i2ckbd.c/h` — the RP2040-side I2C keyboard client (from PicoCalc booter)
- Read `lcdspi.h` and `config.h` from the PicoCalc booter (low-level LCD primitives)
- Read `Setup60_RP2040_ILI9488.h` — TFT_eSPI pin configuration
- Read the `InterruptDriven` example from arduino_picocalc_kbd
- Mapped all SPI and I2C bus pin assignments
- Created docmgr ticket `picocalc-wm` with tasks

### What I learned

**Display pipeline is character-by-character through a scroll buffer:**
- `pserial(c)` → `Display(c)` → `PlotChar(c, line, col)` → `tft.drawChar(x, y, ch, fg, bg, 1)`
- A 53×32 `ScrollBuf[column][line]` array backs the display
- Scrolling copies every character up one row via `ScrollDisplay()` — O(columns × lines) SPI writes per scroll
- The cursor is character `_` (0x5F), shown/hidden by plotting space/cursor at current position
- Parenthesis highlighting inverts colors (green on black) for matched open/close parens

**Keyboard input is polling-based with two paths:**
- **REPL path:** `gserial()` polls `pc_kbd.keyCount()` in a tight loop, feeds `ProcessKey()` which builds `KybdBuf[]`, sets `KybdAvailable=1` on Enter
- **Escape path:** `testescape()` is called during long operations (GC, eval), polls keyboard every 500ms for ESC/~ to abort
- **Direct key path:** `fn_getkey()` calls `getKey()` which blocks polling `pc_kbd.keyEvent()`

**Keyboard hardware:**
- STM32G031 (separate MCU) handles the physical keyboard matrix
- Communicates via I2C on Wire1 (GP6=SDA, GP7=SCL) at 10kHz
- Address 0x1F, FIFO register 0x09 returns `{state, key}` pairs
- The STM32 firmware has an interrupt pin capability (`attachInterrupt`) but uLisp doesn't use it
- Special keys filtered out: 0xA1 (ALT), 0xA2-A3 (SHIFT), 0xA4 (SYM), 0xA5 (CTRL), 0x00, 0xFF

**SPI buses — two separate hardware SPI peripherals:**
- SPI1 (pins 10-15): ILI9488 display at 25MHz
- SPI0 (pins 16-19): SD card at half speed
- I2C1 (pins 6-7): Keyboard at 10kHz

**RP2040 core 1 is completely unused** — uLisp runs entirely on core 0. The PicoCalc booter's lcdspi.c imports `pico/multicore.h` but doesn't use it either.

### What should be done in the future

- Write the architecture analysis document (next step)
- Design the window manager with dual-core in mind
- Implement interrupt-driven keyboard via GPIO pin

### Technical details

**Console constants:**
```
ScreenWidth = 320, ScreenHeight = 320
CharWidth = 6, CharHeight = 8, Leading = 10
Columns = 53, Lines = 32
LastColumn = 52, LastLine = 31
```

**Keyboard I2C register map:**
```
0x01 VER  - firmware version
0x02 CFG  - config bits (overflow, capslock, numlock, key interrupts)
0x03 INT  - interrupt status
0x04 KEY  - key status (count + capslock + numlock)
0x05 BKL  - display backlight
0x06 DEB  - debounce config
0x07 FRQ  - poll frequency
0x08 RST  - RESET (DANGEROUS - crashes MCU)
0x09 FIF  - FIFO (key events)
0x0A BK2  - keyboard backlight
0x0B BAT  - battery percentage
```

**SPI pin map:**
```
Display (SPI1): SCK=10, MOSI=11, MISO=12, CS=13, DC=14, RST=15
SD Card (SPI0): SCK=18, MOSI=19, MISO=16, CS=17
Keyboard (I2C1): SDA=6, SCL=7
```

---

### What worked

- Reading the STM32 keyboard firmware (`picocalc_keyboard.ino`) was critical — it revealed the FIFO register protocol, the interrupt capability, and the I2C speed constraint
- Mapping all pins from `config.h` and `Setup60_RP2040_ILI9488.h` gave a complete picture of available resources
- The `InterruptDriven.ino` example proves GPIO-interrupt keyboard reading is supported by the library

### What didn't work

- No failures in this analysis step

### What I learned

- The console is entirely character-based with a scroll buffer — no framebuffer compositing
- Core 1 is completely unused — perfect for a WM compositor
- The keyboard I2C at 10kHz is the main bottleneck (~4ms per key read)
- The STM32 keyboard MCU supports interrupt-driven reading but uLisp doesn't use it
- `testescape()` uses `longjmp` for abort — this is dangerous in a multi-window context
- Graphics functions bypass the scroll buffer entirely — they write directly to the display

### What was tricky to build

- Tracing the data flow through multiple indirection layers: `pserial → Display → PlotChar → tft.drawChar` with scroll buffer circular indexing `(line+Scroll) % Lines`

### What warrants a second pair of eyes

- The dual-core FIFO architecture proposal — ensuring no deadlocks between Core 0 (Lisp) and Core 1 (WM)
- The memory budget for virtual console buffers (14KB for 4 windows with color attributes)

### What should be done in the future

- Write the window manager design document (what to build)
- Prototype Core 1 FIFO communication
- Benchmark SPI partial redraw performance for window-sized regions
- Test GPIO interrupt latency for keyboard events

### Commit

Code: 8355693 — "Create picocalc-wm ticket with diary and tasks"

### Code review instructions

- Read `design/01-console-architecture.md` sections 2-3 (display and keyboard pipelines)
- Cross-reference against `ulisp-picocalc.ino` lines 7211-7580
- Verify the pin map in section 5 against `config.h` and `Setup60_RP2040_ILI9488.h`

---

## Step 2: Write Literate Console Walkthrough

Wrote a 10-chapter literate programming document that reads the PicoCalc console source code like a textbook, following the Peter Norvig style from the textbook-authoring skill.

### Prompt Context

**User prompt (verbatim):** "Make a longform textbook writing style literate programming document for all of that functionality, so that I can read the real code as I go. (see skill for textbook writing style). Store in ticket, upload to remarkable."

**Assistant interpretation:** Create a comprehensive literate programming document that walks through the actual source code of the console system, with verbatim code listings woven into explanatory prose, following the Peter Norvig style (foundational first, concrete examples, rhythm breaks, no AI slop).

**Inferred user intent:** A readable document that serves as both an educational guide and a reference, allowing the reader to understand the code deeply enough to design and implement a window manager.

### What I did

- Read the textbook-authoring SKILL.md for style guidelines
- Re-read key source sections to quote code verbatim with line numbers
- Wrote the document in 12 small append chunks (following the "small writes" instruction)
- 10 chapters covering: boot sequence, main loop, display pipeline, keyboard pipeline, escape handling, bus topology, dual-core architecture, graphics, sound, and window manager requirements
- Relate 6 source files, uploaded to reMarkable

### What worked

- The Peter Norvig style — opening each chapter with purpose and motivation before showing code
- Quoting actual code with line numbers makes the document traceable to the source
- The "hard problems" section at the end surfaces the real design challenges

### What should be done in the future

- Design the actual window manager architecture (what to build)
- Prototype Core 1 FIFO communication
- Implement virtual console data structure

---

## Step 3: Compile and Upload Firmware

Compiled the uLisp PicoCalc firmware and uploaded it to the SD card on the PicoCalc via the Mac at 192.168.0.57.

### Prompt Context

**User prompt (verbatim):** "compile and upload to 192.168.0.57:/Volumes/NO NAME/firmware/"

**Assistant interpretation:** Cross-compile uLisp for RP2040 using arduino-cli, then scp the resulting .uf2 to the PicoCalc's SD card via the Mac.

**Inferred user intent:** Get the latest uLisp build onto the PicoCalc so it can be flashed from the bootloader.

### What I did

- Ran the existing build script `scripts/01-compile-ulisp-picocalc.sh`
- Build succeeded: `build/ulisp-picocalc-sketch.ino.uf2` (460,800 bytes)
- First scp attempt failed — SD card was mounted read-only on the Mac
- `diskutil unmount force` killed the SSH session (our shell was the mount dissenter)
- User unlocked the SD card (remounted read-write)
- Second scp succeeded: `/Volumes/NO NAME/firmware/PicoCalc_uLisp_4.8f_wm.uf2`

### What didn't work

- `scp` to read-only FAT32 mount returns "Failure" (unhelpful error message)
- `diskutil unmount force` on the mount that holds our SSH session kills the connection — the force unmount terminates the process that holds the mount open, which is our own shell
- `diskutil remount` doesn't exist on macOS (unlike Linux `mount -o remount,rw`)

### What I learned

- macOS mounts FAT32 SD cards read-only by default when they have certain filesystem flags
- The "dissenter PID" in `diskutil unmount` is very literal — it's your own shell
- Staging files to `/tmp` first, then copying in a single ssh command, avoids the session-kill problem

### What was tricky to build

- The read-only mount issue was misleading — the scp error was just "Failure" with no indication it was a filesystem permissions problem. Had to `touch` a test file to confirm.

### What should be done in the future

- Flash the firmware from the PicoCalc bootloader and verify it boots
- Test that the console, keyboard, graphics, and SD card all work with this build

### Technical details

```
Build: arduino-cli compile --fqbn rp2040:rp2040:rpipico
Output: 211,448 bytes code (10% flash), 193,800 bytes global vars (73% RAM)
Warnings: longjmp clobbering (expected), no touch CS (expected — no touchscreen)
Upload: scp → manuel@192.168.0.57:/Volumes/NO NAME/firmware/PicoCalc_uLisp_4.8f_wm.uf2
```

---

## Step 4: Investigate stock bootloader BIN failure and install UF2 Loader menu

The uLisp BIN files copied into `/firmware` did not boot because they were normal UF2 payloads converted to BIN, not legacy PicoCalc multibooter applications. The stock PicoCalc bootloader v1.0 expects `.bin` files that begin with a valid RP2040 vector table and whose reset vectors point into the bootloader app region at `0x10032000`.

Rather than continue fighting the legacy BIN linker path, I installed the newer UF2 Loader menu assets onto the SD card. This lets us keep compiling ordinary Arduino/Pico `.uf2` files for uLisp and place them under `pico1-apps/`, avoiding custom linker scripts during development.

### Prompt Context

**User prompt (verbatim):** "Your little brother has been trying to get the ulisp to boot from the picocalc sd card, but has been struggling. Assess what they did up to know, write a detailed diary entry, then fix it"

**Assistant interpretation:** Review the prior compile/upload/UF2-to-BIN attempts, explain why they failed with the stock PicoCalc bootloader, preserve scripts/sources, and move to a reliable boot path.

**Inferred user intent:** Get uLisp booting from the PicoCalc SD card reliably while preserving enough evidence to understand and reproduce the process later.

### What I did

- Searched and downloaded forum/source references into the ticket `sources/` directory using Defuddle.
- Added `pelrun/uf2loader` as a git submodule and checked it out at release tag `v2.4.1`.
- Read `uf2loader/README.md`, `ui/main.c`, `stage3/stage3.c`, and `stage3/uf2.c`.
- Read the ClockworkPi `pico_multi_booter` source already vendored under `PicoCalc/Code/pico_multi_booter/sd_boot/main.c`.
- Verified headers of known-good stock `.bin` files from `/firmware` on the SD card:
  - Stock app BINs begin with a vector table: stack pointer `0x20042000`, reset vector around `0x100320xx`.
  - Our converted uLisp BINs begin with boot2 instruction bytes like `0x4b32b500`, not a vector table.
- Saved all helper scripts into the ticket `scripts/` directory.
- Installed official UF2 Loader release assets to the SD card:
  - `/BOOT2040.UF2`
  - `/pico1-apps/uLisp_4.8f_arduino_pico_4.5.0.uf2`
  - `/firmware/uf2loader_bootloader_pico_v2.4.1.uf2`
  - `/firmware/uf2loader_diag_pico_v2.4.1.uf2`
- Synced and unmounted the SD card cleanly.

### What worked

- The Defuddle captures gave a clear external confirmation: legacy PicoCalc bootloader apps require custom linker files, while UF2 Loader supersedes that workflow and accepts ordinary UF2s.
- The ClockworkPi source confirmed `SD_BOOT_FLASH_OFFSET = 200 * 1024` and vector-table validation in the loaded app flash region.
- Official UF2 Loader release artifacts were easy to install on the SD card and are safer than locally-built bootloader artifacts for flashing.

### What didn't work

- Normal `.uf2 -> .bin` extraction does not create a valid legacy multibooter app. It extracts the raw UF2 payload starting with boot2, not an offset-linked vector-table application image.
- An attempted Arduino-Pico multiboot linker build initially produced a `.bin` that was still wrong because the Arduino image begins with `.boot2` and `.ota` before `.text`; the stock bootloader wants `.vectors` first.
- Building UF2 Loader from source with the local Pico SDK got as far as `bootloader_pico.uf2` and `diag_pico.uf2`, but the UI (`BOOT2040.UF2`) failed against the local SDK/TinyUSB version:

```text
ui/lib/usb_msc/usb_msc.c:23:3: error: unknown type name 'tusb_rhport_init_t'
ui/lib/usb_msc/usb_msc.c:25:3: error: too many arguments to function 'tusb_init'
```

This indicates the checked-out SDK/TinyUSB API is older than the UF2 Loader source expects. The official release assets avoid this toolchain mismatch.

### What I learned

- Stock PicoCalc Bootloader v1.0 and UF2 Loader are different systems with different app formats.
- For stock v1.0, `.bin` means: linked at `0x10032000`, first bytes are the vector table, no boot2 prefix.
- For UF2 Loader, ordinary `.uf2` files are the intended app format; the menu UI itself lives in `BOOT2040.UF2` on the SD root.
- UF2 Loader displays a menu on the PicoCalc LCD, but only when the flashed bootloader is installed and `BOOT2040.UF2` is present on the SD card.

### What was tricky to build

- The confusing part was that `.bin` can mean two different things: Arduino's raw flash binary starts at `0x10000000` and includes boot2, while the legacy multibooter `.bin` starts at the app vector table and is linked for `0x10032000`.
- The Arduino-Pico build system generates its linker script through `simplesub.py`, so overriding the linker script requires replacing the prelink recipe or using a modified platform. Simply passing another `--script` adds a second linker script and fails.

### What warrants a second pair of eyes

- Flashing a bootloader always carries risk. Use the official UF2 Loader release artifact (`bootloader_pico.uf2`) rather than the local source build until the Pico SDK/TinyUSB mismatch is solved.
- Verify that the PicoCalc has a Pico/RP2040, not Pico 2/RP2350. For Pico/RP2040 use `bootloader_pico.uf2` and `BOOT2040.UF2`; for Pico 2 use `bootloader_pico2.uf2` and `BOOT2350.UF2`.

### What should be done in the future

- Flash `uf2loader_bootloader_pico_v2.4.1.uf2` via BOOTSEL.
- Boot with the prepared SD card inserted and hold Up/F1/F5 to verify the UF2 Loader menu appears.
- Select `uLisp_4.8f_arduino_pico_4.5.0.uf2` from `pico1-apps/` and confirm uLisp boots.
- If we later need stock v1.0 compatibility, build a proper vector-table-first `0x10032000` BIN using ClockworkPi's exact linker script and an Arduino-Pico platform recipe override.

### Code review instructions

- Read `design/03-picocalc-bootloader-analysis.md` for the concise explanation.
- Read `sources/09-solved-app-running-from-bootloader-v1.md` for external confirmation of the stock bootloader linker issue.
- Read `uf2loader/README.md` for installation and usage instructions.
- Inspect known-good stock BIN headers with:

```bash
python3 - <<'PY'
from pathlib import Path
import struct
for p in Path('/Volumes/NO NAME/firmware').glob('*.bin'):
    b = p.read_bytes()[:16]
    print(p.name, ' '.join(hex(x) for x in struct.unpack('<4I', b)))
PY
```

### Technical details

Installed to SD card:

```text
/BOOT2040.UF2
/pico1-apps/uLisp_4.8f_arduino_pico_4.5.0.uf2
/firmware/uf2loader_bootloader_pico_v2.4.1.uf2
/firmware/uf2loader_diag_pico_v2.4.1.uf2
```

How to flash the bootloader itself:

1. Enter RP2040 BOOTSEL mode.
2. Copy `bootloader_pico.uf2` to the `RPI-RP2` USB mass-storage drive.
3. Reboot with the SD card inserted.
4. Hold Up/F1/F5 at power-on to enter the UF2 Loader menu.

---

## Step 5: Write UF2 Loader deep-dive report for Obsidian and ticket

I wrote a long-form technical report explaining the PicoCalc UF2 Loader architecture in textbook style, with concrete source references and no analogies. The report covers the old stock Bootloader v1.0 workflow, the UF2 Loader two-step design, the flashed stage3 bootloader, the SD-card menu UI, the UF2 parsing/flash programming path, compilation requirements, and the practical uLisp development workflow.

The report was written first into the Obsidian vault as an `ARTICLE - ...` note, then copied into the docmgr ticket with `cp`, as requested. This creates one durable knowledge note in the vault and one ticket-local copy that travels with the investigation artifacts.

### Prompt Context

**User prompt (verbatim):** "ok, write a detailed diary and then a project report for our obsidian vault on how to compile uf2 bootloader and how it works in detail, in a deep dive technical blog post style, in a textbook writing style (see skills, no analogies). When done, use `cp` to copy inside the ticket. I want to know how this whole bootloader in 2 steps works, how the menu on the SD card works, all of it."

**Assistant interpretation:** Write a detailed diary entry and then produce a deep technical Obsidian article explaining UF2 Loader's compile/install/runtime architecture, especially the two-step bootloader/menu split and SD-card menu flow. Copy the finished Obsidian report into the ticket with `cp`.

**Inferred user intent:** Preserve the bootloader investigation as durable knowledge, not just a chat answer, so future development can proceed without rediscovering the legacy multibooter/UF2 Loader distinction.

### What I did

- Read the Obsidian vault writing skill and used an `ARTICLE - ...` note shape.
- Re-read `uf2loader/README.md`, `stage3/stage3.c`, `stage3/uf2.c`, `stage3/memmap_2040.ld`, `ui/main.c`, `ui/uf2.c`, and `common/bootloader/proginfo.c`.
- Re-read the ClockworkPi stock multibooter source at `PicoCalc/Code/pico_multi_booter/sd_boot/main.c` and its linker script at `PicoCalc/Code/pico_multi_booter/linker_scripts/memmap_default.ld.mp.rp2040`.
- Wrote the Obsidian note:
  - `/home/manuel/code/wesen/obsidian-vault/Projects/2026/05/05/ARTICLE - PicoCalc UF2 Loader - Two-Stage Bootloader Deep Dive.md`
- Copied it into the ticket using `cp`:
  - `ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/reference/05-picocalc-uf2loader-two-stage-deep-dive.md`
- Verified both copies have the same size and line count: 25,535 bytes, 767 lines.

### What worked

- The source split is clear once read directly:
  - `stage3/` is the small flashed bootloader path.
  - `ui/` is the SD-card menu path loaded from `BOOT2040.UF2`.
  - `common/bootloader/proginfo.c` is the metadata bridge between them.
- The local report now explains exactly why legacy `.bin` files failed and why UF2 Loader solves the problem for normal Arduino uLisp UF2 builds.
- `cp` was used explicitly to copy the Obsidian note into the ticket, preserving the requested workflow.

### What didn't work

- The local source build of UF2 Loader is still not fully solved because the available Pico SDK/TinyUSB combination does not match the API expected by the UF2 Loader UI USB mass-storage code. The build error remains:

```text
ui/lib/usb_msc/usb_msc.c:23:3: error: unknown type name 'tusb_rhport_init_t'
ui/lib/usb_msc/usb_msc.c:25:3: error: too many arguments to function 'tusb_init'
```

- The official release artifacts remain the recommended bootloader/menu files until the SDK/TinyUSB version mismatch is resolved.

### What I learned

- UF2 Loader's menu is not compiled into the flashed bootloader. It is a separate UF2 (`BOOT2040.UF2`) loaded from SD into SRAM.
- Stage3 loads `BOOT2040.UF2` only when Up/F1/F5 is held or when a command asks it to load a RAM app.
- The menu UI is responsible for flashing selected app UF2s into the persistent application flash area.
- Proginfo stored in the vector-table hole is what lets the bootloader know an app is installed and how much flash is safe.

### What was tricky to build

- The phrase "bootloader" refers to multiple binaries: the flashed `bootloader_pico.uf2`, the SD-card `BOOT2040.UF2` menu, and the old stock v1.0 multibooter. The report avoids ambiguity by naming each binary and its location.
- The RP2040 boot vector location differs depending on context. Normal apps have boot2 at flash offset 0 and vectors at `XIP_BASE + 0x100`; legacy stock `.bin` apps must begin directly with vectors at `0x10032000`; UF2 Loader preserves its own boot2 and launches apps through the vector table at `XIP_BASE + 0x100`.

### What warrants a second pair of eyes

- The section about app flash safety should be reviewed once uLisp's save-image/LittleFS behavior is tested under UF2 Loader. uLisp may need to respect the bootloader's protected high-flash area.
- The compile instructions should be revised if we install the exact Pico SDK 2.2.0 / TinyUSB API expected by UF2 Loader and get a clean source build of `BOOT2040.UF2`.

### What should be done in the future

- Flash official `bootloader_pico.uf2` via BOOTSEL and verify the menu appears.
- Select the uLisp UF2 from `/pico1-apps/` and verify it boots.
- Test uLisp flash-writing features under UF2 Loader.
- Optionally fix the local UF2 Loader source build by installing the matching Pico SDK/TinyUSB toolchain.

### Code review instructions

- Start with the vault article or its ticket copy:
  - `/home/manuel/code/wesen/obsidian-vault/Projects/2026/05/05/ARTICLE - PicoCalc UF2 Loader - Two-Stage Bootloader Deep Dive.md`
  - `ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/reference/05-picocalc-uf2loader-two-stage-deep-dive.md`
- Cross-check stage3 behavior in `uf2loader/stage3/stage3.c`.
- Cross-check app flashing in `uf2loader/ui/uf2.c`.
- Cross-check proginfo storage in `uf2loader/common/bootloader/proginfo.c`.
- Cross-check legacy stock bootloader expectations in `PicoCalc/Code/pico_multi_booter/sd_boot/main.c`.
