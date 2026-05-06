---
Title: 'Postmortem: Exploring uLisp on the PicoCalc'
Ticket: ulisp-picocalc
Status: active
Topics:
    - embedded
    - lisp
    - rp2040
    - picocalc
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: PicoCalc/Code/picocalc_keyboard/reg.h
      Note: Keyboard I2C register map
    - Path: arduino_picocalc_kbd/src/PCKeyboard.cpp
      Note: Keyboard I2C library implementation
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/examples/01-geometric-shapes.lisp
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/examples/02-i2c-keyboard.lisp
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/01-compile-ulisp-picocalc.sh
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/02-build-ulisp-native-linux.sh
    - Path: ulisp-picocalc/Setup60_RP2040_ILI9488.h
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: PicoCalc uLisp source v4.8f
    - Path: ulisp-wasm/c99/ulisp.c
      Note: C99 port source
ExternalSources: []
Summary: Full postmortem of the uLisp PicoCalc exploration session — what we set out to do, what actually happened, what went wrong, what we learned, and what the state of the project is now.
LastUpdated: 2026-05-05T20:00:00-04:00
WhatFor: Complete record of the investigation for onboarding and future reference
WhenToUse: Read this first when resuming work on uLisp + PicoCalc
---


# Postmortem: Exploring uLisp on the PicoCalc

## What We Set Out to Do

The goal was simple: explore uLisp on the Clockwork Pi PicoCalc. Compile the firmware, get it running, understand the system. What started as a build exercise turned into a full investigation spanning cross-compilation, native host builds, I2C hardware probing, and the discovery of an entire ecosystem we did not know existed.

This postmortem records the full arc — what we planned, what actually happened, where we stumbled, and what the landscape looks like now.

---

## The Narrative Arc

### Phase 1: Build the Firmware (Steps 1-2)

We started by researching the ecosystem. The PicoCalc is a kit-built handheld computer from Clockwork Pi, based on an RP2040 Raspberry Pi Pico, with a 320x320 ILI9488 IPS display, a QWERTY keyboard driven by a separate STM32 processor over I2C, and an SD card slot. uLisp is a Lisp interpreter for microcontrollers by David Johnson-Davies.

Our first mistake was cloning the wrong repository. The ClockworkPi documentation described a patch-based workflow: clone `ulisp-arm`, pin to version 4.5a, apply a patch from the PicoCalc repo. We followed those instructions and got the patch applied (with whitespace warnings). Then the user correctly pointed out that there is a dedicated repository — `technoblogy/ulisp-picocalc` — which is the author's own PicoCalc fork, already patched and current at version 4.8f. We switched to that.

The toolchain setup was straightforward. We installed `arduino-cli` v1.4.1, added the `arduino-pico` board manager URL, installed the RP2040 core v5.6.0, installed TFT_eSPI v2.5.34, cloned the `arduino_picocalc_kbd` keyboard library, and patched TFT_eSPI's `User_Setup_Select.h` to point at the PicoCalc's ILI9488 display configuration. The build completed cleanly in one pass — 460 KB UF2 image, only harmless warnings about missing touchscreen and `setjmp`/`longjmp` clobbering.

We wrote a build script (`scripts/01-compile-ulisp-picocalc.sh`), a textbook-style build guide, a diary, and uploaded everything to reMarkable.


### Phase 2: Native Linux Build (Step 4)

The user asked for a version compiled for the host Linux PC — not the RP2040 target. This led us to discover `ulisp-wasm` by Eliot Akira, a community project that is far more than its name suggests. It is a monorepo containing a pure C99 port of uLisp (the core we used), a WebAssembly build for browsers, a Node.js runtime, a Zig auto-translation, a minimal uLisp Zero in 811 lines of C, and mirrors of all official uLisp sources.

Building the native binary required exactly one command: `clang -std=c99 -lm -O3 -D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D__HAS_RANDOM__=1 -o build/ulisp-cli -I c99 c99/ulisp.c c99/bestline.c`. Zero warnings. 349 KB binary. It ran immediately — arithmetic, factorial, fibonacci, strings, closures all working. We launched it in a tmux session and tested interactively.

We wrote a design document comparing the native build's capabilities against the PicoCalc firmware, and a build script (`scripts/02-build-ulisp-native-linux.sh`).

### Phase 3: Deep Questions (Step 5)

The user asked five specific questions that forced us to dig into the actual artifacts rather than hand-waving. Each question opened a rabbit hole:

**Why is the UF2 twice the size of the BIN?** UF2 wraps every 256 bytes of payload in a 512-byte block. The 512-byte block size matches USB mass storage sectors. The math confirmed exactly: 230,252 bytes / 256 = 900 blocks, 900 x 512 = 460,800 bytes. Matched to the byte.

**What are the benchmark numbers and where from?** From @picolisper on the ClockworkPi forum, uLisp 4.7b. ARM @ 200 MHz runs Tak in 3.4 seconds, Fibonacci in 2.4 seconds. RISC-V on the RP2350 is about 40% slower at the same clock.

**How do you find the BIOS version, and what if it mismatches?** Read I2C register 0x0E from address 0x1F. BIOS 1.2 returns 0, 1.4 returns 1, 1.6 would return its own value. BIOS 1.4 changed the I2C protocol — added power-off register, changed power button behavior — and broke uLisp and MicroPython.

**Where does 193,800 bytes of memory come from?** A static array: `object Workspace[22280]` at 8 bytes per object = 178,240 bytes. That's 92% of the total. The rest is TFT_eSPI, SPI, SD card, LittleFS library globals. On ATmega328, the same interpreter uses only 320 objects x 4 bytes = 1,280 bytes.

**Why do we need to patch the display?** TFT_eSPI supports 50+ displays. It cannot auto-detect which one is connected. The PicoCalc uses an ILI9488 on GPIO 10-15 via SPI1 at 25 MHz with BGR color order. Two files must change: copy the setup file AND edit the selector. Miss either one and the display is blank with no error message.


### Phase 4: Hardware Communication (Steps 6-8)

The user connected the PicoCalc via USB-C and we found the serial console on `/dev/ttyUSB0` at 9600 baud. We launched `screen` in a tmux session, sent a newline, and the uLisp prompt appeared:

```
22280>
```

The workspace size matched our build exactly: 23,000 - 720 (SD card) = 22,280 objects. We sent arithmetic, defined functions, drew graphics. Everything worked. The PicoCalc display showed the same output simultaneously.

We drew geometric shapes — concentric rectangles, circles via a midpoint algorithm implementation, spirograph curves using parametric equations, diamonds, and a colored grid. The drawing was slow but satisfying: each `(draw-pixel ...)` call is an individual SPI transaction to the ILI9488. A full spirograph (3,600 points) took a few seconds. We saved the examples in `examples/01-geometric-shapes.lisp`.

Then we turned to I2C. The keyboard sits at I2C address `0x1F` on `Wire1` (I2C1, GP6/GP7). This was the trickiest discovery: uLisp routes addresses below 128 to `Wire` (I2C0) and addresses 128+ to `Wire1` (I2C1). So the keyboard must be accessed at address `#x9F` (0x1F + 128), not `#x1F`. Our first attempts at `#x1F` returned nil silently — the I2C0 bus has nothing on it.

Once we used `#x9F`, everything lit up. We read all safe registers and identified our keyboard as **BIOS 1.2** (the VER register at 0x01 returns 0, meaning the version field was not implemented in that firmware revision). We confirmed I2C writes by turning on the display backlight (register 0x05) and the keyboard backlight (register 0x0A).

**The crash.** During a register dump, we read register 0x08 (RST). This is the keyboard reset register. Reading it apparently triggered a reset of the STM32 keyboard processor, which hung the I2C bus and crashed the uLisp REPL. The PicoCalc needed a power cycle to recover. This was the one hard failure of the session, and it taught us to never probe that register.

We saved the I2C examples in `examples/02-i2c-keyboard.lisp` with the RST register clearly marked as dangerous.

### Phase 5: Ecosystem Investigation

We investigated the keyboard firmware ecosystem in depth. The keyboard firmware source lives in `PicoCalc/Code/picocalc_keyboard/` — an Arduino sketch for the STM32F103, compiled with STM32duino. The register map is defined in `reg.h`. There are three released BIOS versions (1.2, 1.4, 1.6) as binary blobs in `PicoCalc/Bin/`.

We discovered a register map mismatch between the keyboard firmware and the `arduino_picocalc_kbd` library. The registers 0x01-0x0B are identical in both, but starting at 0x0C they diverge: the library has GPIO passthrough registers (DIR, PUE, PUD, GIO, GIC, GIN) while the current firmware source has C64 matrix, joystick, and power-off registers. The library was written by @cuu (the ClockworkPi developer) but has only 2 commits and was not updated for BIOS 1.4+. Basic keyboard reading works because it only uses registers 0x01-0x0B.

We vendored all dependencies as git submodules: `ulisp-picocalc`, `ulisp-wasm`, `PicoCalc`, and `arduino_picocalc_kbd`. The keyboard library is symlinked into `~/Arduino/libraries/` so the cross-compilation build still works.


---

## What Went Wrong

### 1. Cloned the Wrong Repository

We followed the ClockworkPi documentation, which describes patching `ulisp-arm` v4.5a. The dedicated `ulisp-picocalc` repo is newer, cleaner, and at v4.8f. The user caught this immediately. The lesson: check for dedicated forks before following patch-based instructions.

### 2. The Register 0x08 Crash

Reading I2C register 0x08 (RST) crashed the keyboard MCU and hung the system. There is no protection in the uLisp I2C interface against reading dangerous registers. The fix is purely procedural: document which registers are safe and never probe 0x08. We added a warning to the I2C example file.

### 3. The Wire1 Routing Quirk

The keyboard is on I2C1 (Wire1), but at address 0x1F which is below 128. uLisp's convention routes addresses >= 128 to Wire1. So the keyboard must be accessed at 0x9F. This is not documented anywhere — we discovered it by reading the uLisp source code (line 7320: `Wire1.setSDA(6); Wire1.setSCL(7); pc_kbd.begin(0x1f,&Wire1);`) and then checking the `with-i2c` routing logic (line 3151: `if (address > 127) port = &Wire1;`). The initial attempts at `#x1F` returned nil silently because I2C0 has no devices on it.

### 4. Large File Writes

The first version of the textbook guide was written as a single 26 KB write, which the user flagged. Subsequent documents were written incrementally using small `cat >>` appends. This is a process lesson: for ticket documents, always append in chunks.

---

## What Went Right

### 1. First-Pass Build Success

The cross-compilation build worked on the first try. No missing libraries, no wrong versions, no cryptic errors. The arduino-cli toolchain setup was smooth. TFT_eSPI patching was done correctly the first time.

### 2. Native Build Discovery

Finding `ulisp-wasm` was a genuine breakthrough. A single clang command produces a fully working uLisp REPL on Linux. This gives us a development environment for testing Lisp logic without hardware. The 349 KB binary compiled with zero warnings.

### 3. Hardware Communication

Getting the serial console working was immediate — plug in USB-C, screen at 9600 baud, and the uLisp prompt appears. Drawing graphics and probing I2C registers from the REPL was satisfying and productive. The combination of tmux + screen + send-keys made it possible to interact programmatically with the hardware.

### 4. Documentation Trail

We downloaded 19 source documents via defuddle, wrote 2 build scripts, 2 example Lisp files, a diary, a build guide, a design doc, a Q&A doc, and this postmortem. Everything is tracked in the docmgr ticket and four documents are on reMarkable. The documentation overhead was significant but means the work is fully reproducible.


---

## The Current State of the Project

### What We Have

| Artifact | Location | Status |
|----------|----------|--------|
| PicoCalc firmware (460 KB UF2) | `build/ulisp-picocalc-sketch.ino.uf2` | Built, flashed, running on hardware |
| Native Linux uLisp (349 KB ELF) | `ulisp-wasm/build/ulisp-cli` | Built, tested, running in tmux |
| PicoCalc running uLisp 4.8f | Serial `/dev/ttyUSB0` at 9600 baud | Live in tmux session `serial` |
| Native uLisp 4.8g | Linux host | Live in tmux session `ulisp` |

### Submodules Vendored

```
ulisp-picocalc/       → technoblogy/ulisp-picocalc    (uLisp PicoCalc source, v4.8f)
ulisp-wasm/           → eliot-akira/ulisp-wasm        (C99 port, WASM, examples)
PicoCalc/             → clockworkpi/PicoCalc          (hardware docs, keyboard firmware source, BIOS binaries)
arduino_picocalc_kbd/ → cuu/arduino_picocalc_kbd      (keyboard I2C library, symlinked to Arduino)
```

### Key Technical Findings

| Finding | Detail |
|---------|--------|
| Keyboard BIOS version | **BIOS 1.2** (VER register = 0) — fully compatible with uLisp |
| I2C routing quirk | Keyboard at 0x1F on Wire1 — must use address **#x9F** in uLisp |
| Dangerous register | **0x08 (RST) crashes keyboard** — never read or write it |
| Display backlight | I2C register 0x05, value 0-255 |
| Keyboard backlight | I2C register 0x0A, value 0-255 |
| Workspace | 22,280 objects (178 KB) on RP2040 Pico |
| UF2 overhead | Exactly 2x the raw binary (256 payload bytes per 512-byte block) |
| Native uLisp | 65,536 objects, compiles with one clang command, zero dependencies |

### Register Map (BIOS 1.2)

```
0x01 VER  Firmware version (0 on BIOS 1.2)
0x02 CFG  Configuration flags
0x03 INT  Interrupt status
0x04 KEY  Key count + lock states
0x05 BKL  Display backlight (0-255)
0x06 DEB  Debounce config
0x07 FRQ  Poll frequency config
0x08 RST  Reset *** DO NOT TOUCH ***
0x09 FIF  Key FIFO (read key events)
0x0A BK2  Keyboard backlight (0-255)
0x0B BAT  Battery status
0x0C-0x0E C64/Joystick/Power-off (BIOS 1.4+ only, undefined on 1.2)
```


---

## What Should Be Done Next

### Short Term

- **Run the benchmarks** from `sources/05-clockworkpi-ulisp-4-7b-benchmarks.md` on our PicoCalc and compare against the reported numbers. This tells us whether our core v5.6.0 build matches the tested v4.5.0 performance.
- **Save a screenshot** using `(save-bmp "test.bmp")` to capture the geometric shapes we drew.
- **Test SD card access** — try `(directory)` and `(save-image)` to verify LittleFS and SD card.
- **Read keyboard events** via the FIFO register (0x09) — write a Lisp function that waits for a keypress using the I2C interface rather than the built-in `get-key`.

### Medium Term

- **Update `arduino_picocalc_kbd`** for BIOS 1.6 compatibility — the register map from 0x0C onward needs updating. This would help the broader community since the library is still the standard way to talk to the PicoCalc keyboard.
- **Write test suites** that run on the native Linux uLisp, then port to PicoCalc. The 60+ examples in `ulisp-wasm/examples/` are a rich source of test programs.
- **Explore the browser WASM playground** at `eliot-akira.github.io/ulisp-wasm` — could be useful for tutorials or documentation.

### Long Term

- **Upgrade to Pico 2** (RP2350) — nearly doubles the workspace (47,000 objects) and adds a faster ARM Cortex-M33 core.
- **Contribute back** to `arduino_picocalc_kbd` or `ulisp-picocalc` if we find fixes or improvements.
- **Build a Lisp development workflow** — edit on host, test on native, deploy to PicoCalc via UF2.

---

## Sources Archived

All 19 source documents are in `sources/`:

| # | File | Source |
|---|------|--------|
| 01 | github-technoblogy-ulisp-picocalc.md | GitHub README |
| 02 | ulisp-picocalc-machine-doc.md | ulisp.com official PicoCalc page |
| 03 | clockworkpi-picocalc-ulisp-build-instructions.md | ClockworkPi build instructions |
| 04 | ulisp-forum-picocalc-lisp-machine.md | uLisp forum announcement |
| 05 | clockworkpi-ulisp-4-7b-benchmarks.md | Benchmark numbers by @picolisper |
| 06 | clockworkpi-ulisp-official-support.md | ClockworkPi forum thread |
| 07 | ulisp-main-page.md | ulisp.com main page with all platform versions |
| 08 | ulisp-port-to-c-webassembly.md | uLisp forum: C99/WASM port announcement |
| 09 | ulisp-porting-new-platform.md | ulisp.com: porting guide |
| 10 | github-eliot-akira-ulisp-wasm.md | ulisp-wasm GitHub README |
| 11 | picocalc-bios-1-4-keyboard-firmware.md | ClockworkPi forum: BIOS 1.4 changes |
| 12 | ulisp-main-page-platforms.md | ulisp.com: full platform table |
| 13 | ulisp-i2c-spi-interface.md | ulisp.com: I2C/SPI API reference |
| 14 | ulisp-objects.md | ulisp.com: objects and workspace |
| 15 | ulisp-performance.md | ulisp.com: performance benchmarks |
| 16 | ulisp-tail-call-optimization.md | ulisp.com: TCO implementation |
| 17 | ulisp-implementation.md | ulisp.com: implementation details |
| 18 | picocalc-keyboard-arduino-setup.md | ClockworkPi wiki: keyboard dev setup |
| 19 | picocalc-custom-keyboard-firmware.md | ClockworkPi forum: custom firmware |

---

## Documents on reMarkable

All uploaded to `/ai/2026/05/05/ulisp-picocalc/`:

1. **01-diary** — Investigation diary (Steps 1-5)
2. **02-building-ulisp-picocalc-guide** — Textbook-style build guide (13 sections)
3. **02-running-ulisp-natively-on-linux** — Native C99 port research doc
4. **03-questions-and-deep-answers** — Five deep-dive Q&A chapters
