---
Title: Running uLisp Natively on Linux
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
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/02-build-ulisp-native-linux.sh
      Note: Build script for native binary
    - Path: ulisp-wasm/c99/bestline.c
    - Path: ulisp-wasm/c99/readme.md
    - Path: ulisp-wasm/c99/ulisp.c
      Note: C99 port of uLisp
ExternalSources: []
Summary: Research and design doc for running uLisp as a native Linux binary using the ulisp-wasm C99 port, for development and testing without PicoCalc hardware.
LastUpdated: 2026-05-05T19:20:00-04:00
WhatFor: Reference for building and using uLisp on a Linux host for PicoCalc development
WhenToUse: When you want to test uLisp code on your Linux machine before flashing to PicoCalc
---


# Running uLisp Natively on Linux

## Why Run uLisp on Your Host Machine?

The PicoCalc is a wonderful device, but developing on it has friction. You type on a small keyboard, read output on a 320x320 screen, and every change requires flashing firmware. Running uLisp natively on your Linux machine removes all of that friction. You get a full-sized keyboard, a terminal window, copy-paste, and instant feedback. You can prototype Lisp functions, test logic, and iterate fast — then port the working code to the PicoCalc.

This is the same pattern used in every embedded workflow: develop on the host, deploy to the target.

---

## The Discovery: ulisp-wasm

There is no official "uLisp for desktop Linux" from the uLisp author. The official versions all target microcontrollers via the Arduino IDE. But the community has produced something arguably better: a pure C99 port of uLisp that compiles with standard system compilers and runs anywhere.

The project is [ulisp-wasm](https://github.com/eliot-akira/ulisp-wasm) by Eliot Akira. Despite the name suggesting WebAssembly, the C99 core (`c99/ulisp.c`) is a clean, dependency-light port that compiles natively on Linux, macOS, and Windows. It was adapted from the ESP32 version of uLisp and gradually rewritten to remove all Arduino dependencies.

### What it is

- A single C source file (`c99/ulisp.c`, ~9000 lines) plus a readline library (`c99/bestline.c`)
- Based on uLisp ESP 4.8g
- Compiles with `clang` or `gcc` — no Arduino IDE, no Docker, no Emscripten needed for native builds
- Produces a ~350 KB native ELF binary
- Runs an interactive REPL with line editing and history (via bestline)
- Passes the full uLisp test suite (640 tests)

### What it is not

- It does not have PicoCalc-specific extensions (no `get-key`, no `draw-pixel`, no `save-bmp`)
- It does not have hardware I/O (no I2C, no SPI, no analog read/write)
- It does not have SD card or LittleFS support in native mode
- Graphics support requires Adafruit GFX libraries (Arduino-only)

These are acceptable trade-offs. The value of the native build is testing pure Lisp logic — functions, recursion, list manipulation, math — not hardware interaction.

---

## Building the Native Binary

### Prerequisites

You need `clang` installed. On Ubuntu:

```bash
sudo apt install clang
```

That is it. No other dependencies.

### The Build Command

```bash
cd ulisp-wasm
mkdir -p build
clang -std=c99 -lm -O3 \
    -D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D__HAS_RANDOM__=1 \
    -o build/ulisp-cli \
    -I c99 \
    c99/ulisp.c c99/bestline.c
```

Let's break down the flags:

| Flag | Purpose |
|------|----------|
| `-std=c99` | Compile as C99 (the source uses C99 features like `bool`, `stdint.h`) |
| `-lm` | Link the math library (needed for `fmod`, `sqrt`, floating-point ops) |
| `-O3` | Optimize for speed |
| `-D_DEFAULT_SOURCE` | Enable `fchmod()` from `<sys/stat.h>` |
| `-D_XOPEN_SOURCE` | Enable `fileno()` from `<stdio.h>` |
| `-D__HAS_RANDOM__=1` | Use `random()` instead of `rand()` for better randomness |
| `-I c99` | Find `bestline.h` header |
| `c99/ulisp.c c99/bestline.c` | The two source files (interpreter + line editor) |

### Build Result

```
-rwxrwxr-x 349K build/ulisp-cli
ELF 64-bit LSB pie executable, x86-64
```

A 349 KB single-file binary. No shared libraries beyond libc and libm.

---

## Running the REPL

```bash
./build/ulisp-cli
```

You see:

```
uLisp 4.8g
> 
```

The REPL is fully interactive. It uses bestline (a BSD-licensed readline replacement) for line editing, history (up/down arrows), and basic tab completion.

### Quick Smoke Test

```
> (+ 1 2 3 4 5)
15

> (defun fact (n) (if (= n 0) 1 (* n (fact (- n 1)))))
fact

> (fact 10)
3628800

> (fact 20)
2.4329e18

> (defun fib (n) (if (< n 3) 1 (+ (fib (- n 1)) (fib (- n 2)))))
fib

> (fib 20)
6765
```

### Running in tmux

For development, run uLisp in a tmux session so it persists:

```bash
tmux new-session -s ulisp "./build/ulisp-cli"
```

Send commands programmatically:

```bash
tmux send-keys -t ulisp '(+ 1 2)' Enter
```

Capture output:

```bash
tmux capture-pane -t ulisp -p
```

### Piping Input

You can also pipe Lisp expressions into uLisp for batch testing:

```bash
cat test.lisp | ./build/ulisp-cli
```

---

## Differences from PicoCalc uLisp

Understanding what differs between the native build and the PicoCalc firmware helps you know what you can test locally and what must be tested on hardware.

### Shared Core

Both versions share the same uLisp interpreter core. This means these features behave identically:

- All standard Lisp forms (`defun`, `let`, `cond`, `loop`, `dotimes`, `dolist`...)
- Arithmetic and math (`+`, `-`, `*`, `/`, `mod`, `sqrt`, `sin`, `cos`...)
- List operations (`car`, `cdr`, `cons`, `list`, `mapcar`, `assoc`...)
- String operations
- Closures and lexical scoping
- Tail-call optimization
- Garbage collection
- Error handling (setjmp/longjmp)
- `(save-image)` / `(load-image)` — uses filesystem on native, flash/SD on PicoCalc

### Native-Only Features

- Larger workspace: 65,536 objects (vs 23,000 on Pico, 47,000 on Pico 2)
- No memory pressure — 64K objects = ~512 KB, trivial on a modern PC
- Full IEEE 754 floating point (no precision loss)
- Line editing with bestline (history, navigation)
- File-based save/load image

### PicoCalc-Only Features (not in native build)

- `get-key` — keyboard input
- `read-pixel` / `draw-pixel` — display operations
- `save-bmp` — screen capture to SD card
- `note` — sound via PWM
- TFT_eSPI graphics (all `draw-*` and `fill-*` functions)
- SD card via SPI
- LittleFS filesystem
- ARM assembler (`defcode`)
- I2C/SPI hardware interfaces

---

## Architecture of the C99 Port

The C99 port was created by taking the ESP32 Arduino source (`ulisp-esp.ino`) and systematically replacing Arduino-specific calls with standard C/POSIX equivalents.

| Arduino | C99/POSIX Replacement |
|---------|---------------------|
| `Serial.print()` | `printf()` / `putchar()` |
| `Serial.read()` | `getchar()` / `bestline()` |
| `delay(ms)` | `struct timespec` + `nanosleep()` |
| `millis()` | `clock_gettime(CLOCK_MONOTONIC)` |
| `random()` | `random()` (via `-D__HAS_RANDOM__` flag) |
| `EEPROM` / `Flash` | `fopen()`/`fwrite()` file I/O |
| Arduino `String` | C `char*` with `strlen`/`strcpy` |
| `PROGMEM` / `PSTR()` | Empty macros (no Harvard architecture on host) |

The workspace allocation uses `malloc()` instead of static arrays, and the workspace size defaults to 65,536 objects — roughly 8x what a PicoCalc provides.

---

## Cross-Platform Builds

The `ulisp-wasm` build system (in `build.ts`) supports cross-compilation via Zig's C compiler:

```bash
# Requires Zig and Bun installed
bun build.ts build:cli
```

This produces binaries for:

- `ulisp-cli-linux-x64`
- `ulisp-cli-linux-arm64`
- `ulisp-cli-macos-x64`
- `ulisp-cli-macos-arm64`
- `ulisp-cli-windows-x64`
- `ulisp-cli-riscv64-linux-gnu`
- `ulisp-cli-riscv64-linux-musl`

The Windows builds omit bestline (no readline on Windows) and lose the interactive REPL.

---

## Recommendations for PicoCalc Development

### What to test on native uLisp

- Pure algorithmic code (sorting, math, list processing)
- Function definitions and macro-like patterns
- String manipulation
- File I/O (using the POSIX filesystem as a stand-in for SD card)
- Test suites and regression tests

### What you must test on hardware

- Anything involving `get-key`, `draw-pixel`, `read-pixel`, `save-bmp`
- Sound (`note`)
- SD card specifics (path names, card formatting)
- Display layout (the native build has no 320x320 constraint)
- Performance and memory limits (23K objects vs 65K objects)
- ARM assembler routines

### Workflow Suggestion

1. Develop and test Lisp logic in the native REPL
2. Port to PicoCalc, adapting hardware-specific calls
3. Flash and verify on hardware
4. Iterate

---

## Related

- Source: `ulisp-wasm/` submodule (cloned from `eliot-akira/ulisp-wasm`)
- Build script: `scripts/02-build-ulisp-native-linux.sh`
- C99 source: `ulisp-wasm/c99/ulisp.c`
- Line editor: `ulisp-wasm/c99/bestline.c`
- Forum thread: [uLisp port to C and WebAssembly](http://forum.ulisp.com/t/ulisp-port-to-c-and-webassembly/1729)
- Online demo: [eliot-akira.github.io/ulisp-wasm](https://eliot-akira.github.io/ulisp-wasm/)
