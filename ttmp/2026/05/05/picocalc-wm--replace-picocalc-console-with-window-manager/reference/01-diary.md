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
