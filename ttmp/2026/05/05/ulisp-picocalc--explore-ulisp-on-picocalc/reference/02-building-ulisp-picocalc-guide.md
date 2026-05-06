---
Title: 'Building uLisp for the PicoCalc: A Hands-On Guide'
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
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/01-compile-ulisp-picocalc.sh
      Note: Build script
    - Path: ulisp-picocalc-sketch/ulisp-picocalc-sketch.ino
    - Path: ulisp-picocalc/Setup60_RP2040_ILI9488.h
      Note: Display setup header
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Main uLisp PicoCalc source
ExternalSources: []
Summary: Textbook-style guide covering everything an intern needs to understand and reproduce the uLisp PicoCalc firmware build, from hardware fundamentals through compilation.
LastUpdated: 2026-05-05T19:15:00-04:00
WhatFor: Onboarding reference for building embedded Lisp firmware for the PicoCalc
WhenToUse: Read this before attempting to build uLisp for PicoCalc, or to understand the embedded toolchain
---


# Building uLisp for the PicoCalc: A Hands-On Guide

## What You Will Understand After Reading This

This guide explains how to turn a Clockwork Pi PicoCalc — a kit-built handheld computer — into a self-contained Lisp machine by compiling and flashing custom firmware. By the end, you will understand what each component of the build toolchain does, why it exists, and how to reproduce the build from scratch on a Linux machine. The goal is not to memorize commands, but to see the embedded firmware build as a coherent system of responsibilities, each playing a specific role.

---

## 1. The Hardware: What Is a PicoCalc?

The Clockwork Pi PicoCalc is a handheld computer that ships as a kit. It takes about fifteen minutes to assemble — no soldering required, just mechanical assembly following an illustrated booklet. Once assembled, it boots into BASIC. Our job is to replace that firmware with something more interesting: a Lisp interpreter.

The PicoCalc contains three processors, and understanding why there are three is the first step to understanding the build.

### The Main Processor: RP2040

The heart of the PicoCalc is a Raspberry Pi Pico board, which is built around the RP2040 microcontroller. The RP2040 is a dual-core ARM Cortex M0+ running at 125 MHz (though it can be overclocked to 200 MHz). It provides 264 KB of on-chip SRAM and supports up to 16 MB of off-chip flash memory. The Raspberry Pi Pico specifically has 2 MB of flash.

Why does this matter? The RP2040 is the processor that runs our Lisp interpreter. Every uLisp function call, every garbage collection cycle, every pixel drawn to the screen — it all happens on this chip. The 264 KB of RAM is the Lisp workspace, the space where your Lisp objects live. The 2 MB of flash holds both the compiled firmware (our UF2 image) and a filesystem for saving Lisp programs.

### The Display: ILI9488 via SPI

The PicoCalc has a 320×320 pixel, 4-inch IPS color display driven by an ILI9488 controller chip. The display connects to the RP2040 via SPI (Serial Peripheral Interface), a synchronous serial protocol that is the workhorse of embedded display communication.

The ILI9488 is not a "smart" display — it does not understand text. It understands pixels. When uLisp prints a character to the screen, the firmware must look up that character's pixel pattern in a font table and write those pixels to the display one by one. The TFT_eSPI library handles this translation.

### The Keyboard: STM32 via I2C

Here is a design decision worth sitting with: the keyboard is not connected directly to the RP2040. Instead, the PicoCalc has a separate STM32F103 microcontroller dedicated to scanning the keyboard matrix. The STM32 reads which keys are pressed and communicates the results to the RP2040 over I2C (Inter-Integrated Circuit), a two-wire serial bus.

Why separate the keyboard onto its own processor? A keyboard matrix requires constant scanning — checking every row and column intersection tens of times per second to detect key presses. Offloading this work to a dedicated chip means the main processor never has to interrupt its Lisp evaluation to poll for keys. The RP2040 simply asks "any keys?" over I2C when it's ready, and the STM32 answers. This is a clean separation of concerns implemented in hardware.

### The Other Components

The PicoCalc also has an SD card slot (connected via SPI), a speaker (driven by PWM on GPIO pins 26 and 27), and runs on one or two 18650 lithium batteries charged through the USB-C port. These are all accessible from uLisp: the SD card for file storage, the speaker for playing notes, and the USB-C port doubles as a serial connection for programming from a computer.

---

## 2. The Software: What Is uLisp?

uLisp is a subset of Common Lisp designed to run on microcontrollers with limited memory. It was created by David Johnson-Davies and is maintained at ulisp.com. The name is a play on μLisp — the Greek letter mu for "micro."

### Why Lisp on a Microcontroller?

Lisp is one of the oldest programming languages, and it has a property that makes it uniquely suited to small computers: Lisp programs are Lisp data. A Lisp function is a list of symbols and numbers, and the Lisp interpreter evaluates lists. This homoiconicity (code is data) means the interpreter itself can be surprisingly compact — there is no separate parser, compiler, and runtime. There is just eval, applied to lists.

uLisp takes advantage of this. The entire interpreter — reader, evaluator, garbage collector, printer, and extensions for graphics, SD cards, and ARM assembly — fits in about 210 KB of compiled code. That leaves room in the RP2040's flash for a filesystem, and enough RAM for roughly 23,000 Lisp objects (each object is 8 bytes, so about 184 KB of the 264 KB RAM).

### The Source Code

The uLisp PicoCalc source is a single C++ file: `ulisp-picocalc.ino`, approximately 7,800 lines. The `.ino` extension is the Arduino sketch format. Despite being a single file, it contains the complete Lisp system:

- **Reader**: Parses text input (from keyboard or serial) into Lisp objects
- **Evaluator**: Walks the object tree and executes functions
- **Printer**: Converts Lisp objects back to text for display
- **Garbage collector**: Reclaims memory from objects no longer in use
- **PicoCalc extensions**: Keyboard input (`get-key`), pixel operations (`read-pixel`, `draw-pixel`), screen capture (`save-bmp`), sound (`note`)

### Compile-Time Configuration

At the top of the source file, you will find a set of `#define` directives that configure what gets included in the build:

```cpp
// #define resetautorun       // Auto-run saved workspace on boot
#define printfreespace        // Show free memory at prompt
#define serialmonitor         // Enable serial monitoring
// #define printgcs            // Debug: print garbage collection stats
#define sdcardsupport         // Enable SD card file operations
#define gfxsupport            // Enable graphics (required for PicoCalc)
// #define lisplibrary         // Include a pre-loaded Lisp library
#define assemblerlist         // Enable ARM assembler listing
// #define extensions          // Include uLisp extensions file
```

Each `#define` is a toggle. Comment it out with `//` and that feature disappears from the compiled firmware, freeing memory for Lisp objects. The `gfxsupport` line is critical for PicoCalc — without it, there is no display driver.

---

## 3. The Toolchain: How Does Source Code Become Firmware?

This is the part that confuses most newcomers, so let's walk through it carefully. When we type `arduino-cli compile`, a chain of tools transforms human-readable C++ source into a binary image the RP2040 can execute. Understanding this chain means you can diagnose any build failure.

### Step 1: The Preprocessor

The C preprocessor handles all lines starting with `#`. When it sees `#include <TFT_eSPI.h>`, it finds that header file and pastes its contents into the source. When it sees `#define sdcardsupport`, it creates a flag. When it sees `#if defined(sdcardsupport)`, it includes or excludes code based on that flag. The preprocessor produces a single, expanded C++ file.

### Step 2: The Compiler

The ARM GCC compiler (`arm-none-eabi-g++`) translates C++ into ARM machine code. But not just any ARM machine code — it generates Thumb-2 instructions, which is the compact instruction set used by the Cortex M0+. The compiler also optimizes: removing unused functions, inlining small functions, and choosing the most efficient instruction sequences.

### Step 3: The Linker

The linker takes all the compiled object files (your code, the TFT_eSPI library, the Arduino core, the RP2040 SDK) and combines them into a single ELF binary. It resolves function calls — connecting your call to `tft.fillScreen()` to the actual machine code in the TFT_eSPI library. It also places code and data at specific memory addresses defined by the RP2040's memory map.

### Step 4: The Object Copy

The ELF binary contains debugging symbols and metadata that the RP2040 does not need. The `objcopy` tool strips this down to a raw `.bin` file — just the bytes that go into flash memory.

### Step 5: UF2 Conversion

The RP2040 has a clever bootloader: when you hold the BOOTSEL button and connect USB, it appears as a USB mass storage drive (like a USB thumb drive). You can copy a specially formatted file onto this drive, and the bootloader writes it to flash. That special format is UF2 (USB Flashing Format), developed by Microsoft for exactly this purpose. The build tools wrap the raw binary in UF2 headers so the RP2040 bootloader can process it.

---

## 4. The Build Environment: Setting Up From Scratch

Now that you understand what each component does, let's set up the build environment. The instructions target a Linux machine. Every command is explained.

### 4.1 Install arduino-cli

The Arduino IDE is a graphical application. For automated builds, we use `arduino-cli`, the command-line version. It does everything the IDE does, but from a terminal.

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
  | BINDIR=~/.local/bin sh
```

This downloads the latest `arduino-cli` binary and places it in `~/.local/bin`. After installation, verify it works:

```bash
arduino-cli version
# Expected: arduino-cli  Version: 1.4.1 (or later)
```

### 4.2 Initialize the Configuration

```bash
arduino-cli config init
```

This creates a configuration file at `~/.arduino15/arduino-cli.yaml`. The configuration tells arduino-cli where to find board definitions and libraries.

### 4.3 Add the RP2040 Board Definition

The Arduino ecosystem organizes board support into "cores." The default core supports AVR-based Arduino boards. For the RP2040, we need a third-party core maintained by Earle Philhower:

```bash
arduino-cli config set board_manager.additional_urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

This tells arduino-cli: "When looking for boards, also check this URL." The URL points to a JSON index file that lists all available RP2040-based boards and the tools needed to compile for them.

### 4.4 Install the RP2040 Core

```bash
arduino-cli core update-index    # Download the board index files
arduino-cli core install rp2040:rp2040   # Install the core and toolchain
```

This is the heavy step. It downloads the ARM GCC cross-compiler (about 110 MB), the RP2040 SDK, the PIO assembler, OpenOCD for debugging, and other tools. The total download is roughly 350 MB. The installed core version at the time of writing was 5.6.0.

### 4.5 Install the Display Library: TFT_eSPI

The PicoCalc's ILI9488 display needs a driver library. TFT_eSPI by Bodmer is the de facto standard for SPI-connected TFT displays in the Arduino ecosystem:

```bash
arduino-cli lib install TFT_eSPI@2.5.34
```

Why version 2.5.34 specifically? Because that is the version the uLisp author tested with. In embedded development, pinning library versions is standard practice. A newer version might change API behavior or memory layout, and on a device with 264 KB of RAM, even small regressions matter.

### 4.6 Patch TFT_eSPI for the PicoCalc Display

TFT_eSPI supports hundreds of displays. You tell it which display you have by editing a configuration header. This is the step most people get wrong, so let's be precise.

TFT_eSPI uses a file called `User_Setup_Select.h` as a selector. By default, it includes `User_Setup.h`, which is a generic configuration. We need to replace this with the PicoCalc-specific configuration:

```bash
# Copy the PicoCalc display setup into TFT_eSPI's setup directory
cp ulisp-picocalc/Setup60_RP2040_ILI9488.h \
   ~/Arduino/libraries/TFT_eSPI/User_Setups/
```

Then edit `~/Arduino/libraries/TFT_eSPI/User_Setup_Select.h`:

```cpp
// Comment out the default:
// #include <User_Setup.h>           // Default setup is root library folder

// Add the PicoCalc setup:
#include <User_Setups/Setup60_RP2040_ILI9488.h>
```

The `Setup60_RP2040_ILI9488.h` file tells TFT_eSPI: the display controller is an ILI9488, the SPI clock pin is GPIO 10, the data pin is GPIO 11, the chip select is GPIO 9, the data/command pin is GPIO 8, and the reset pin is GPIO 12. These are the specific GPIO pins the PicoCalc hardware wires to the display. If any of these are wrong, the display will show nothing — no error message, just silence.

### 4.7 Install the Keyboard Library

```bash
cd ~/Arduino/libraries
git clone https://github.com/cuu/arduino_picocalc_kbd.git
```

This library provides the `PCKeyboard` class that talks to the STM32 keyboard processor over I2C. Without it, the firmware will not compile (it references `PCKeyboard pc_kbd` at the top of the source).

---

## 5. The Build: Compiling the Firmware

### 5.1 Preparing the Sketch

In Arduino terminology, a "sketch" is a project directory. The directory name must match the main `.ino` file name. We create a sketch directory and copy the source:

```bash
mkdir -p ulisp-picocalc-sketch
cp ulisp-picocalc/ulisp-picocalc.ino ulisp-picocalc-sketch/ulisp-picocalc-sketch.ino
```

### 5.2 The Compile Command

```bash
arduino-cli compile \
    --fqbn rp2040:rp2040:rpipico \
    --build-path ./build \
    --warnings all \
    ./ulisp-picocalc-sketch
```

Let's break this down:

- `--fqbn rp2040:rp2040:rpipico` — The Fully Qualified Board Name. This tells the compiler: use the `rp2040` core (from Earle Philhower), the `rp2040` architecture, and the `rpipico` board variant. Other valid boards include `rpipicow` (Pico W with WiFi), `rpipico2` (Pico 2 with RP2350), and `rpipico2w` (Pico 2 W).

- `--build-path ./build` — Where to put the compiled output.

- `--warnings all` — Show all compiler warnings. In embedded development, warnings often indicate real bugs.

### 5.3 What to Expect: The Output

A successful build produces output like this:

```
Sketch uses 211448 bytes (10%) of program storage space. 
Maximum is 2093056 bytes.
Global variables use 193800 bytes (73%) of dynamic memory, 
leaving 68344 bytes for local variables. Maximum is 262144 bytes.
```

These numbers tell a story. The firmware uses 10% of the 2 MB flash — plenty of room. But the dynamic memory usage is 73% of the RP2040's RAM. That 193,800 bytes is the Lisp workspace: the memory pool where Lisp objects (numbers, symbols, cons cells, strings) are allocated. The remaining 68,344 bytes are for the stack (function call frames, local variables) and the TFT frame buffer.

### 5.4 Expected Warnings (Harmless)

The build produces a few warnings that are safe to ignore:

**Warning 1: `TOUCH_CS pin not defined`**

```
#warning >>>>------>> TOUCH_CS pin not defined, 
TFT_eSPI touch functions will not be available!
```

The PicoCalc has no touchscreen. The TFT_eSPI library warns you that touch functions are disabled. This is expected behavior, not an error.

**Warning 2: `variable 'result' might be clobbered by 'longjmp'`**

```
warning: variable 'result' might be clobbered by 'longjmp' or 'vfork'
```

uLisp uses `setjmp`/`longjmp` for error handling — when a Lisp error occurs, `longjmp` jumps back to the top-level REPL without unwinding the stack properly. The compiler warns that local variables in functions that contain `setjmp` points might have stale values after a `longjmp`. This is a known trade-off in uLisp's design: the error handling is simple, and the variables in question are always re-initialized after the jump.

### 5.5 The Build Artifacts

After compilation, the `build/` directory contains:

| File | Size | Purpose |
|------|------|---------|
| `ulisp-picocalc-sketch.ino.bin` | 230 KB | Raw binary (flash contents) |
| `ulisp-picocalc-sketch.ino.uf2` | 460 KB | UF2-wrapped binary (copy to Pico) |
| `ulisp-picocalc-sketch.ino.elf` | ~1 MB | Full ELF with debug symbols |
| `ulisp-picocalc-sketch.ino.map` | ~500 KB | Memory map (symbol addresses) |

The `.uf2` file is what you flash to the PicoCalc. The `.elf` and `.map` files are useful for debugging with GDB or OpenOCD but are not needed for normal use.

---

## 6. Flashing: Getting Firmware onto the Device

The RP2040 bootloader makes firmware installation remarkably simple:

1. Disconnect the PicoCalc from USB if connected.
2. Hold the BOOTSEL button on the Raspberry Pi Pico board (accessible through a slot on the back of the PicoCalc marked "reset").
3. While holding BOOTSEL, connect the Pico's micro-USB port to your computer.
4. Release BOOTSEL. The Pico appears as a USB drive named `RPI-RP2`.
5. Copy the `.uf2` file onto this drive.
6. The drive ejects automatically. The firmware is now flashed.

After flashing, hold the PicoCalc power button for one second. You should see the uLisp prompt (`> `) on the display, and you can start typing Lisp expressions at the keyboard.

---

## 7. Two Approaches to the Source: Which One to Use?

During our research, we found two ways to get the uLisp PicoCalc source code. Understanding the difference teaches you something about how open-source firmware projects evolve.

### Approach A: The Dedicated Repository

```
git clone https://github.com/technoblogy/ulisp-picocalc.git
```

This is the author's dedicated repository. It contains a single `.ino` file that is ready to compile. The PicoCalc-specific code (display driver, keyboard handling, sound) is already integrated. This is what we used for our build.

### Approach B: Patching the Generic Source

```
git clone https://github.com/technoblogy/ulisp-arm.git
cd ulisp-arm
git reset --hard 97e6115    # Pin to version 4.5a
git clone https://github.com/clockworkpi/PicoCalc.git
git apply PicoCalc/Code/uLisp/uLisp.patch
```

This was the original method, documented by Clockwork Pi. You take the generic ARM version of uLisp (which supports dozens of boards) and apply a patch that adds PicoCalc-specific code. The patch is a diff file — it says "at line X, add these lines; at line Y, change this to that."

We started with Approach B before realizing Approach A existed. Approach A is simpler and more current. Approach B is useful if you want to understand exactly what changed between generic uLisp and the PicoCalc version — just read the patch file.

---

## 8. Platform Support: Which Boards Work?

The uLisp source code contains conditional compilation blocks for each supported board. The workspace size varies significantly depending on the board:

| Board | Chip | Workspace | Objects | Notes |
|-------|------|-----------|---------|-------|
| Raspberry Pi Pico | RP2040 | 23,000 | ~184 KB | Default, included with PicoCalc |
| Raspberry Pi Pico W | RP2040 | 15,230 | ~122 KB | WiFi support reduces RAM |
| Raspberry Pi Pico 2 | RP2350 (ARM) | 47,000 | ~376 KB | More RAM on RP2350 |
| Raspberry Pi Pico 2 | RP2350 (RISC-V) | 42,500 | ~340 KB | RISC-V core uses more stack |
| Raspberry Pi Pico 2W | RP2350 (ARM) | 39,200 | ~314 KB | WiFi reduces available RAM |
| Pimoroni Pico Plus 2 | RP2350 + PSRAM | 1,000,000 | ~8 MB | With PSRAM enabled |

The workspace size is measured in Lisp objects. Each object is 8 bytes (two 32-bit pointers on ARM). A PicoCalc with the default Pico board gives you about 23,000 objects minus 720 for the SD card buffer, which is generous enough for most Lisp programs.

The Pico 2 with its RP2350 chip is an attractive upgrade: nearly double the workspace. The ARM core benchmarks roughly 30% faster than the RP2040 at the same clock speed, and the RISC-V core is about 30% slower. At 200 MHz overclock, the ARM RP2350 runs the Tak benchmark in 3.4 seconds; the RP2040 at its stock 150 MHz takes 4.6 seconds.

---

## 9. Serial Communication: Talking to a Computer

The PicoCalc can be connected to a computer via USB for a more comfortable development experience. By default, uLisp uses `Serial1`, which is the USB-C port on the PicoCalc (not the Pico's micro-USB port). This means you connect a USB-C cable from the PicoCalc to your computer and open a serial monitor at 9600 baud.

If you prefer to use the Pico's micro-USB port instead, comment out this line in the source:

```cpp
// #define Serial Serial1
```

This redirects serial communication to the Pico's own USB port. The trade-off: the USB-C port also charges the battery, so using Serial1 means your battery charges while you develop.

---

## 10. Key Concepts Recap

Let's consolidate what matters most from this build:

- **The PicoCalc has three processors.** The RP2040 runs your code. The STM32 handles keyboard scanning. The ILI9488 drives the display. Each has a clear responsibility, and they communicate via SPI (display) and I2C (keyboard).

- **The build toolchain is a pipeline.** Preprocessor expands includes and defines → compiler generates ARM Thumb-2 machine code → linker combines everything into an ELF → objcopy strips to raw binary → UF2 wrapper makes it flashable. A failure at any stage produces different symptoms.

- **Library version pinning matters.** TFT_eSPI 2.5.34 was tested by the author. A different version might work, but on embedded systems, "might work" is not the same as "works." The same applies to the RP2040 core version.

- **The TFT_eSPI patch is the most fragile step.** Forget to copy the setup file, or forget to edit `User_Setup_Select.h`, and the build will either fail with undefined pin errors or succeed but display nothing on the hardware. Both files must be changed.

- **UF2 is the flashing format.** The RP2040 bootloader presents itself as a USB drive. Copy the UF2 file, and you are done. No programmer hardware needed, no JTAG, no SWD. This is one of the RP2040's best design features.

- **Memory is the constraint.** With 73% of RAM used by the Lisp workspace, there is not much headroom. Disable features you don't need (SD card, assembler) to reclaim space for Lisp objects.

---

## 11. Troubleshooting Guide

### Build fails with "Board not supported!"

The `--fqbn` flag does not match a supported board. Check that you typed the FQBN correctly and that the RP2040 core is installed.

### Build fails with TFT_eSPI errors about undefined pins

You forgot to patch TFT_eSPI. Both steps are required: copy `Setup60_RP2040_ILI9488.h` to the User_Setups folder AND edit `User_Setup_Select.h`.

### Build succeeds but display is blank on the PicoCalc

The TFT_eSPI patch might have the wrong pin definitions, or you compiled for the wrong board variant. Verify the FQBN matches your actual hardware.

### Build succeeds but keyboard doesn't respond

The `arduino_picocalc_kbd` library is missing from `~/Arduino/libraries/`. Clone it again.

### PicoCalc continuously reboots

This has been reported on forums and is typically caused by a BIOS version mismatch. uLisp is currently only compatible with the original BIOS 1.2 keyboard firmware. If your PicoCalc shipped with BIOS 1.4, you may need to downgrade the keyboard firmware or wait for a uLisp update.

### The UF2 file is different size than the pre-built one

This is normal. Different versions of the RP2040 core produce different binary sizes. What matters is that the build completes without errors.

---

## 12. What We Actually Did: The Build Log

Here is the exact sequence of commands that produced our working firmware, recorded as a reference:

```
# 1. Install arduino-cli
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh

# 2. Configure
arduino-cli config init
arduino-cli config set board_manager.additional_urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core update-index
arduino-cli core install rp2040:rp2040

# 3. Install libraries
arduino-cli lib install TFT_eSPI@2.5.34
cd ~/Arduino/libraries && git clone https://github.com/cuu/arduino_picocalc_kbd.git

# 4. Patch TFT_eSPI
cp ulisp-picocalc/Setup60_RP2040_ILI9488.h ~/Arduino/libraries/TFT_eSPI/User_Setups/
# Edit ~/Arduino/libraries/TFT_eSPI/User_Setup_Select.h:
#   Comment out: #include <User_Setup.h>
#   Add: #include <User_Setups/Setup60_RP2040_ILI9488.h>

# 5. Prepare sketch
mkdir -p ulisp-picocalc-sketch
cp ulisp-picocalc/ulisp-picocalc.ino ulisp-picocalc-sketch/ulisp-picocalc-sketch.ino

# 6. Compile
arduino-cli compile --fqbn rp2040:rp2040:rpipico --build-path ./build ./ulisp-picocalc-sketch

# Result: build/ulisp-picocalc-sketch.ino.uf2 (460,800 bytes)
```

Total toolchain download: ~500 MB. Total build time on a modern machine: about 90 seconds (first build; subsequent builds are faster due to caching).

---

## 13. Further Reading

- **uLisp PicoCalc Machine**: [ulisp.com/show?56ZO](http://www.ulisp.com/show?56ZO) — The official documentation by the author
- **uLisp Forum — PicoCalc category**: [forum.ulisp.com/c/clockwork-pi-picocalc](http://forum.ulisp.com/c/clockwork-pi-picocalc)
- **Source code**: [github.com/technoblogy/ulisp-picocalc](https://github.com/technoblogy/ulisp-picocalc)
- **PicoCalc hardware repo**: [github.com/clockworkpi/PicoCalc](https://github.com/clockworkpi/PicoCalc)
- **arduino-pico core**: [github.com/earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico)
- **TFT_eSPI library**: [github.com/Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- **Keyboard library**: [github.com/cuu/arduino_picocalc_kbd](https://github.com/cuu/arduino_picocalc_kbd)
