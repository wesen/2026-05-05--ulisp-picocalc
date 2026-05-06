---
Title: 'uLisp PicoCalc: Questions and Deep Answers'
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
    - Path: build/ulisp-picocalc-sketch.ino.bin
    - Path: build/ulisp-picocalc-sketch.ino.uf2
    - Path: ulisp-picocalc/Setup60_RP2040_ILI9488.h
      Note: Display configuration with pin mappings
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Source for WORKSPACESIZE and workspace array definition
ExternalSources: []
Summary: Five fundamental questions about the uLisp PicoCalc build answered in depth, covering UF2 format, benchmarks, BIOS compatibility, memory layout, and display patching.
LastUpdated: 2026-05-05T19:30:00-04:00
WhatFor: Understanding the why behind every layer of the build
WhenToUse: When you want to understand what the build artifacts mean, how memory is laid out, or why the display needs patching
---


# uLisp PicoCalc: Questions and Deep Answers

This document started from five questions that came up after building the firmware. Each question opened into something worth understanding in its own right. The answers are drawn from the source code, the build output, the UF2 specification, and the ClockworkPi forum threads we archived in `sources/`.

---

## 1. Why is the UF2 File Twice the Size of the BIN?

After a successful build, the `build/` directory contains two firmware files:

```
ulisp-picocalc-sketch.ino.bin   230,252 bytes  (225 KB)
ulisp-picocalc-sketch.ino.uf2   460,800 bytes  (450 KB)
```

The `.bin` file is the raw flash content — the exact bytes the RP2040 will execute, laid out address by address. The `.uf2` file is the same content wrapped in the USB Flashing Format (UF2), developed by Microsoft for exactly this use case: making firmware flashing as simple as copying a file to a USB drive.

### The UF2 Block Structure

UF2 does not compress or transform the binary. It divides the raw binary into 512-byte blocks, but only half of each block carries payload. The structure of each block looks like this:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic number `0x0A324655` ("UF2" + newline) |
| 4 | 4 | Magic number `0x9E5D5157` |
| 8 | 4 | Flags |
| 12 | 4 | Target address in flash |
| 16 | 4 | Payload size (always 256) |
| 20 | 4 | Block sequence number |
| 24 | 4 | Total number of blocks |
| 28 | 4 | File size or family ID |
| 32 | 256 | **Payload data** |
| 288 | 224 | Padding / metadata |

The first 32 bytes are the header. The next 256 bytes are the actual firmware data. The remaining 224 bytes are padding. This means every 512-byte block carries exactly 256 bytes of useful content — a fixed 50% overhead ratio.

### Why 512 Bytes?

The RP2040 bootloader presents itself as a USB mass storage device — it looks like a USB thumb drive when you hold BOOTSEL and plug in USB. USB mass storage transfers data in 512-byte sectors. By making each UF2 block exactly one sector, the bootloader can process each block independently as it arrives. There is no need to reassemble blocks, maintain state, or worry about sector alignment. Each sector is self-contained: it carries the target address and the data to write there.

The magic numbers serve a practical purpose. A FAT filesystem also lives on this virtual drive, and operating systems write their own bookkeeping files (`.fseventsd`, `System Volume Information`, etc.). The bootloader ignores anything that does not start with the UF2 magic bytes. This is why you can safely drag a UF2 file onto a drive that also contains hidden OS metadata — the bootloader filters for UF2 blocks and ignores everything else.

### Confirming the Math

Our build produces a 230,252-byte binary. Divided into 256-byte payloads:

```
ceil(230,252 / 256) = 900 blocks
900 × 512 = 460,800 bytes
```

The actual UF2 file is exactly 460,800 bytes. The ratio is not approximately 2:1 — it is precisely 2:1, as guaranteed by the format.


---

## 2. What Are the Reported Benchmark Numbers, and Where Do They Come From?

The benchmark numbers we cited earlier come from a forum post by **@picolisper** on the ClockworkPi forum, in the thread titled "PicoCalc uLisp 4.7b benchmarks and images" (archived in `sources/05-clockworkpi-ulisp-4-7b-benchmarks.md`). This is the same community member who contributed patches to the uLisp author for PicoCalc support.

### The Numbers

| Benchmark | ARM @ 150 MHz | ARM @ 200 MHz | RISC-V @ 150 MHz | RISC-V @ 200 MHz |
|-----------|:---:|:---:|:---:|:---:|
| tak       | 4.6s | 3.4s | 6.6s | 4.8s |
| fib       | 3.2s | 2.4s | 4.9s | 3.6s |
| q         | 6.2s | 4.6s | 9.5s | 6.7s |
| q2        | 11.3s | 8.4s | 16.7s | 12.0s |
| factor    | 1.6s | 1.2s | 2.2s | 1.6s |
| sieve     | 18.1s | 13.6s | 23.6s | 17.1s |

### What the Benchmarks Measure

These are standard uLisp benchmarks, defined on the uLisp website:

- **tak** (Takeuchi function) — a recursive function that tests deep recursion and integer comparison. It is pathologically recursive and stresses the interpreter's function call overhead.
- **fib** (Fibonacci, naive recursive) — the doubly-recursive `(fib n) = (fib n-1) + (fib n-2)`. Tests recursion and arithmetic. Exponential time complexity, so it measures raw interpreter speed on small inputs.
- **q** and **q2** — quicksort on lists. Tests list manipulation (car, cdr, cons, append) plus comparison and recursion.
- **factor** — integer factorization. Tests arithmetic loops and division.
- **sieve** — Sieve of Eratosthenes. Tests array/bitfield operations and loops.

### Reading the Table

The ARM columns are the RP2040 (Cortex M0+) in the standard Pico, and the RP2350 (Cortex M33) in the Pico 2. The RISC-V columns are the RP2350's alternative RISC-V core, which can be selected at build time.

The RP2040's stock clock is 133 MHz, but the arduino-pico core v4.5.0+ certified it at **150 MHz**, which is the "stock" number in the table. The 200 MHz column is overclocking — the RP2040 is known to be stable at this speed, and many PicoCalc users run it overclocked for the ~30% speedup.

The RISC-V core on the RP2350 is interesting: at the same clock speed, it is roughly 40% slower than the ARM core for these benchmarks. This is not a deficiency of RISC-V — it reflects that the RP2350's RISC-V core is a simpler microarchitecture than its ARM Cortex-M33 core.

### Context: What Version Was Tested?

These numbers are for uLisp **4.7b**, which was a pre-release contributed by @picolisper before the author's official PicoCalc support. The current official version in the `ulisp-picocalc` repo is 4.8f. Benchmark numbers should be comparable across minor versions since the interpreter core has not changed significantly.


---

## 3. How Do You Find the BIOS Version, and What Happens If It Mismatches?

This question matters because the PicoCalc's keyboard is not a passive peripheral — it has its own microcontroller (an STM32) running its own firmware, and that firmware has gone through incompatible revisions.

### What the "BIOS" Actually Is

When people say "PicoCalc BIOS," they mean the firmware running on the STM32F103 keyboard processor, not the RP2040 firmware. The STM32 scans the keyboard matrix, manages power, and communicates keypresses to the RP2040 over I2C. The RP2040 (running uLisp or MicroPython or whatever you flashed) talks to the STM32 by reading and writing I2C registers.

There have been at least two versions of this keyboard firmware:

- **BIOS 1.2** — the original version shipped with early PicoCalc units
- **BIOS 1.4** — a newer version that adds a power-off register and treats the power button as a keypress (key code `0x91`)

### How to Detect the Version

You can probe the keyboard's I2C registers. The keyboard sits at I2C address `0x1F` (the standard PicoCalc keyboard address). Reading specific registers tells you the version:

- **Register `0x00`**: Returns `0x00` on official firmware. Custom firmwares use other values (for example, the community custom firmware uses `0xCA`).
- **Register `0x0E`**: Returns `0x00` on BIOS 1.2 and `0x01` on BIOS 1.4. This is the most reliable version indicator.

From uLisp itself, you could detect this with the I2C interface:

```lisp
(with-i2c (str #x1F 2)
  (write-byte #x0E str)
  (restart-i2c str 1)
  (read-byte str))
```

If this returns 0, you have BIOS 1.2. If it returns 1, you have BIOS 1.4.

### What Changed in BIOS 1.4

The changes are documented in the ClockworkPi forum thread we archived (`sources/11-picocalc-bios-1-4-keyboard-firmware.md`). The key changes:

1. **Power-off register** — A new register (`0x0E`) that lets the main processor request a timed shutdown. You write a delay value (minimum 6 seconds) and the keyboard controller cuts power after that delay. This was added to support graceful Linux shutdown on PicoCalc units that use a Linux SBC instead of a Pico.

2. **Power button as keypress** — The power button now generates I2C key event `0x91` instead of just being a hardware power toggle. This lets software detect a long-press and decide what to do (save state, shut down, etc.).

3. **Reset register fix** — The `REG_ID_RST` register was fixed to properly reset the keyboard state.

### What Happens on Mismatch

The uLisp author states in the official documentation: *"uLisp is currently only compatible with the original BIOS 1.2 keyboard firmware."*

On BIOS 1.4, users have reported:

- **Flashing/flickering display** — The changed I2C protocol confuses the keyboard library, causing spurious events that interfere with display updates.
- **Continuous reboot loops** — Some firmware images (MicroPython v1.25, and some uLisp builds) fail to initialize the keyboard correctly and crash on boot, causing the PicoCalc to restart endlessly.

The `arduino_picocalc_kbd` library was written against BIOS 1.2's I2C protocol. BIOS 1.4 changed the register map and event format enough that the library gets confused.

### What To Do About It

If your PicoCalc came with BIOS 1.4 and you want to run uLisp:

1. **Downgrade** the keyboard firmware to BIOS 1.2 using STM32CubeProgrammer (connect to the STM32's SWD pads on the PicoCalc PCB). The BIOS 1.2 binary is available in the `clockworkpi/PicoCalc` GitHub repo under `Bin/`.
2. **Wait** for the uLisp author or the community to update `arduino_picocalc_kbd` for BIOS 1.4 compatibility.
3. **Try it anyway** — some users on BIOS 1.4 report uLisp working partially, depending on the exact firmware version and which features they use.


---

## 4. Where Does 193,800 Bytes of Memory Come From?

When the Arduino build system reports:

```
Global variables use 193,800 bytes (73%) of dynamic memory,
leaving 68,344 bytes for local variables. Maximum is 262,144 bytes.
```

it is talking about **static allocations** — memory reserved at compile time, not allocated at runtime. The "dynamic memory" here means RAM (as opposed to flash), not dynamically allocated memory in the malloc sense. Everything in this 193,800 bytes is either initialized data (the `.data` segment) or zero-initialized data (the `.bss` segment). On the RP2040, all of this lives in the 264 KB of on-chip SRAM.

### The Big One: The Lisp Workspace

The vast majority comes from a single line in the source:

```c
object Workspace[WORKSPACESIZE] WORDALIGNED MEMBANK;
```

This is a static array. For the RP2040 Pico, the preprocessor chain works out to:

```
WORKSPACESIZE = 23000 - 720 = 22,280 objects
sizeof(object) = 8 bytes (on 32-bit ARM: two 32-bit pointers)
Workspace = 22,280 × 8 = 178,240 bytes
```

That 178,240 bytes is the Lisp heap — the memory pool where every Lisp object lives. Every number, every symbol, every cons cell, every string is allocated from this array. When you type `(+ 1 2)` at the REPL, the interpreter carves three objects out of this workspace: one for the number 1, one for the number 2, and one for the cons cell that holds the list `(1 2)`.

### Why a Static Array, Not malloc?

On a microcontroller, you know exactly how much RAM you have, and you want to use as much of it as possible for the Lisp workspace. A static array guarantees the memory is available and contiguous. Using `malloc` would introduce fragmentation risk and uncertainty — you would never be sure exactly how much heap the Lisp interpreter could get, because other libraries and the stack would be allocating from the same pool.

The static approach is simple and deterministic: the compiler lays out the workspace array at a fixed address, the stack grows downward from the top of RAM, and the gap between them is known at link time. The build system checks this gap and warns if it is too small.

### What's in the Other 15,560 Bytes?

The remaining static memory (193,800 - 178,240 = 15,560 bytes) comes from library globals:

- **TFT_eSPI** — SPI transfer buffers, display state, font data. The GLCD font alone is about 1,820 bytes in flash, but the library also keeps a small frame buffer and state in RAM.
- **SPI library** — DMA channel maps, transfer state.
- **SD card library** — File handle state, sector buffers.
- **LittleFS** — Filesystem state for the 1 MB flash partition.
- **Arduino core** — Serial buffers, timer state, GPIO configuration.

These are all the support infrastructure that makes the PicoCalc's hardware accessible to uLisp.

### Why So Much? Doesn't uLisp Run on ATmega328?

Yes, but with a very different workspace size. The key insight is that `WORKSPACESIZE` is not a property of uLisp itself — it is a property of the board it is compiled for. The same interpreter core scales from tiny to generous depending on available RAM.

| Board | RAM | Objects | Object Size | Workspace Bytes | What You Can Do |
|-------|-----|---------|-------------|----------------|-----------------|
| ATmega328 (Arduino Uno) | 2 KB | 320 | 4 bytes | 1,280 | Simple examples, basic math |
| ATmega1284 | 16 KB | 2,944 | 4 bytes | 11,776 | Most uLisp programs |
| RP2040 Pico | 264 KB | 22,280 | 8 bytes | 178,240 | Full programs, SD card, graphics |
| RP2350 Pico 2 (ARM) | 520 KB | 47,000 | 8 bytes | 376,000 | Large programs, more complex data |
| Pimoroni Pico Plus 2 (PSRAM) | ~8 MB | 1,000,000 | 8 bytes | 8,000,000 | Practically unlimited |

On 8-bit AVR platforms (ATmega328), objects are 4 bytes because pointers are 16-bit. On 32-bit ARM (RP2040), objects are 8 bytes because pointers are 32-bit. This is why the same interpreter needs more RAM on ARM — not because it is less efficient, but because each object is twice as large.

The ATmega328's 320 objects is genuinely cramped. You can define a few functions and do basic list manipulation, but you will hit workspace exhaustion quickly. The uLisp website marks most example programs as requiring boards with at least 2,944 objects. The PicoCalc's 22,280 objects is comfortable for most tasks.


---

## 5. Why Do We Need to Patch the Display Library?

The TFT_eSPI library by Bodmer is the standard Arduino library for driving TFT displays over SPI. It supports over 50 different display controllers from a dozen manufacturers. Each controller speaks a slightly different SPI dialect, and each board wires the display to different GPIO pins.

TFT_eSPI cannot auto-detect which display is connected. It relies on compile-time configuration to know: which controller chip am I talking to, on which pins, at what speed, with what color order. This configuration is delivered through two header files.

### The Problem

By default, TFT_eSPI uses a file called `User_Setup.h` that configures a generic display (typically an ILI9341 on ESP8266 pins). If you compile uLisp without patching, TFT_eSPI will generate code for that generic display — wrong controller, wrong pins, wrong color order. The result is one of two outcomes:

1. **Compilation fails** — some pin definitions conflict, or the wrong driver is selected and expected functions are missing.
2. **Compilation succeeds but the display is blank** — the code runs but sends commands to the wrong GPIO pins, or sends ILI9341 commands to an ILI9488 that does not understand them. The display stays dark with no error message.

### The Solution: Two Files to Change

**File 1: `Setup60_RP2040_ILI9488.h`** — This is the PicoCalc-specific display configuration. It tells TFT_eSPI:

```
Driver:     ILI9488_DRIVER
Color:      TFT_BGR (Blue-Green-Red, not RGB — the PicoCalc display swaps R and B)
SPI speed:  25 MHz
Pins:       MISO=12, MOSI=11, SCLK=10, CS=13, DC=14, RST=15
Font:       LOAD_GLCD only (the tiny 8-pixel font, to save flash)
Touch:      Not defined (PicoCalc has no touchscreen)
```

This file is copied into `~/Arduino/libraries/TFT_eSPI/User_Setups/`.

**File 2: `User_Setup_Select.h`** — This is the selector file that TFT_eSPI includes at the top of every compilation. By default it includes `User_Setup.h`. We change it to:

```cpp
// #include <User_Setup.h>           // Commented out — default does not match our hardware
#include <User_Setups/Setup60_RP2040_ILI9488.h>   // Use PicoCalc-specific setup
```

Both changes are required. If you only copy the setup file but do not edit the selector, TFT_eSPI will still include `User_Setup.h` and ignore your PicoCalc configuration entirely. If you only edit the selector but do not provide the setup file, the compiler will fail with "file not found."

### Why Not Auto-Detect?

SPI displays do not have a standard identification mechanism. Some controllers have an ID register you can read, but the read requires SPI to already be configured — which requires knowing the pins. It is a chicken-and-egg problem. The Arduino ecosystem solves this by making display configuration a compile-time decision, which is why every TFT_eSPI-based project starts with a display setup step.

The warning you see during compilation — `TOUCH_CS pin not defined, TFT_eSPI touch functions will not be available!` — is TFT_eSPI's way of confirming that touch functions are disabled. On the PicoCalc, this is correct behavior, not an error.

### The Pin Mapping in Context

Why these specific pins? The PicoCalc PCB routes the RP2040's GPIO to the display connector in a specific pattern:

```
RP2040 GPIO 10  →  ILI9488 SCLK (SPI clock)
RP2040 GPIO 11  →  ILI9488 MOSI (SPI data in, master-out-slave-in)
RP2040 GPIO 12  →  ILI9488 MISO (SPI data out, for reading display memory)
RP2040 GPIO 13  →  ILI9488 CS   (chip select)
RP2040 GPIO 14  →  ILI9488 DC   (data/command selector)
RP2040 GPIO 15  →  ILI9488 RST  (reset)
```

These are not the default SPI0 pins on the RP2040 (which would be GPIO 2-7 for SPI0). The PicoCalc designer chose GPIO 10-15 because those pins are on the RP2040's SPI1 peripheral, which leaves SPI0 free for the SD card. The setup file uses software SPI (bit-banging) rather than hardware SPI — at 25 MHz, the display is fast enough that software SPI is sufficient, and it gives more flexibility in pin choice.

---

## Summary

| Question | Key Insight |
|----------|-------------|
| UF2 vs BIN size | UF2 wraps every 256 bytes of payload in a 512-byte block. The 2x ratio is the format, not bloat. |
| Benchmark numbers | From @picolisper on ClockworkPi forum, uLisp 4.7b. ARM @ 200 MHz is ~30% faster than 150 MHz. |
| BIOS version | Detect via I2C register 0x0E. BIOS 1.4 changed the protocol and breaks uLisp; downgrade to 1.2 if needed. |
| 193,800 bytes memory | 178 KB is the static Lisp workspace array (22,280 objects × 8 bytes). ATmega328 gets only 320 objects × 4 bytes. |
| Display patch | TFT_eSPI must be told which display controller and pins at compile time. Two files must change, not one. |
