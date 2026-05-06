---
Title: 'Playbook: CLion for Arduino/PicoCalc Development'
Ticket: ulisp-picocalc
Status: active
Topics:
    - embedded
    - lisp
    - rp2040
    - picocalc
    - clion
    - tooling
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: .idea/c_cpp_properties.json
      Note: |-
        Generated IntelliSense configuration
        Generated IntelliSense config
    - Path: _ulisp_fwd_decls.h
      Note: |-
        Auto-generated forward declarations
        36 minimal forward declarations for CLion IntelliSense
    - Path: build/compile_commands.json
      Note: Build output used by CLion
    - Path: build/sketch/ulisp-picocalc-sketch.ino.cpp
      Note: Arduino-preprocessed source with forward declarations
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/01-compile-ulisp-picocalc.sh
      Note: Build script that produces compile_commands.json
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/03-generate-c_cpp_properties.py
      Note: |-
        Generator for .idea/c_cpp_properties.json
        Generator script with forward-decl extraction
    - Path: ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/04-extract-forward-declarations.py
      Note: Extracts Arduino forward declarations for CLion
ExternalSources: []
Summary: Step-by-step playbook for configuring JetBrains CLion as a code browser and editor for Arduino/RP2040 firmware development, using the uLisp PicoCalc project as the worked example.
LastUpdated: 2026-05-05T21:00:00-04:00
WhatFor: Setting up CLion for any Arduino/RP2040 project from scratch, or reproducing the uLisp PicoCalc IDE configuration
WhenToUse: Follow this playbook when opening an Arduino .ino project in CLion and you want working IntelliSense, navigation, and build integration
---


# Playbook: Setting Up CLion for Arduino/PicoCalc Development

## What This Playbook Gives You

By the end of this playbook you will have CLion configured as a **code browser and editor** for an Arduino `.ino` project — specifically uLisp on the PicoCalc. You will get:

- **Go-to-Definition** that jumps into `LittleFS.h`, `TFT_eSPI.h`, `hardware/pwm.h`, `Arduino.h`, and every other library header
- **Symbol resolution** for functions like `pserial`, `pln`, `indent`, `printstring` that are forward-referenced in the single `.ino` file
- **Build integration** via `arduino-cli` and External Tools, so you can compile and flash from inside CLion
- **A regeneration script** so the configuration survives toolchain updates

What you will **not** get: CLion debugging the RP2040 in-circuit (that requires OpenOCD + SWD hardware), or CLion acting as a full Arduino IDE replacement. Think of CLion as your **navigator and editor**; the actual build is still `arduino-cli`.

---

## The Problem: Why CLion Doesn't Work Out of the Box

Arduino projects present three specific challenges to CLion's C++ IntelliSense:

### Challenge 1: No CMake

CLion's native build system is CMake. Arduino projects have no `CMakeLists.txt`. Instead, the build is orchestrated by `arduino-cli` (or the Arduino IDE), which generates compiler flags dynamically based on the board, libraries, and variant selected. CLion needs to be told those flags.

### Challenge 2: The `.ino` Preprocessing Gap

Arduino `.ino` files are not valid C++. The Arduino builder preprocesses them: it generates forward declarations for every function, adds `#include <Arduino.h>`, and renames to `.cpp`. A 7,793-line `.ino` becomes an 8,721-line `.cpp` with 462 forward declarations injected. CLion parses the raw `.ino` and sees forward references to functions defined 6,000 lines later. Result: red squiggles everywhere.

### Challenge 3: The `-iprefix` and `@file` Tricks

The RP2040 Arduino core (`arduino-pico` by Earle Philhower) uses GCC extensions heavily. It passes hundreds of include directories via `-iprefix` and response files (`@platform_def.txt`, `@platform_inc.txt`, `@core_inc.txt`). CLion's IntelliSense engine does not expand these — each must be listed explicitly.

---

## Prerequisites

Before starting, verify these are installed and working:

```
# arduino-cli with RP2040 core
arduino-cli version              # 1.4.1+
arduino-cli board listall | grep -i "Raspberry Pi Pico$"
# → Raspberry Pi Pico    rp2040:rp2040:rpipico

# RP2040 core installed
ls ~/.arduino15/packages/rp2040/hardware/rp2040/
# → 5.6.0  (or whatever version)

# Libraries installed
ls ~/Arduino/libraries/TFT_eSPI/TFT_eSPI.h     # must exist
ls ~/Arduino/libraries/arduino_picocalc_kbd/src/PCKeyboard.h  # must exist

# Cross-compiler
ls ~/.arduino15/packages/rp2040/tools/pqt-gcc/*/bin/arm-none-eabi-g++
```

If any of these are missing, follow the [build guide](02-building-ulisp-picocalc-guide.md) Steps 1-3 first.

---

## Step 1: Compile Once to Produce `compile_commands.json`

The entire CLion setup feeds off one file: `build/compile_commands.json`. This is the JSON Compilation Database — a standard format that records the exact compiler flags used for every source file.

```
cd /path/to/your/project
bash ttmp/.../scripts/01-compile-ulisp-picocalc.sh
```

Or directly:

```
arduino-cli compile \
    --fqbn rp2040:rp2040:rpipico \
    --build-path ./build \
    --warnings all \
    ./ulisp-picocalc-sketch
```

**Verify it exists:**

```
python3 -c "import json; cmds=json.load(open('build/compile_commands.json')); print(f'{len(cmds)} compilation entries')"
# → 240 compilation entries
```

You should also see the preprocessed source:

```
ls -la build/sketch/ulisp-picocalc-sketch.ino.cpp
# → ~380KB, the Arduino-preprocessed .ino with forward declarations
```

**When to redo this step:** Every time you change libraries, board variant, or core version. The `compile_commands.json` captures a snapshot of the build flags — it's not auto-updated.

---

## Step 2: Generate `c_cpp_properties.json`

Run the generator script. It reads `compile_commands.json`, expands all `-iprefix` and `@file` entries, walks the pico-sdk tree, and produces the IntelliSense configuration:

```
python3 ttmp/.../scripts/03-generate-c_cpp_properties.py
```

**Expected output:**

```
  Forward declarations: 36 extracted to _ulisp_fwd_decls.h
Generated .idea/c_cpp_properties.json
  181 include paths
  105 defines
  compiler: ~/.arduino15/packages/rp2040/tools/pqt-gcc/4.1.0-1aec55e/bin/arm-none-eabi-g++
```

**What this produces:**

| File | Purpose |
|------|---------|
| `.idea/c_cpp_properties.json` | 181 include paths, 105 defines, compiler path, forced include |
| `_ulisp_fwd_decls.h` | 36 minimal forward declarations (of 462 Arduino-generated) |

**Verify the critical headers resolve:**

```
python3 -c "
import json
with open('.idea/c_cpp_properties.json') as f:
    data = json.load(f)
incs = data['configurations'][0]['includePath']
for name in ['LittleFS', 'TFT_eSPI', 'hardware_pwm', 'arduino_picocalc_kbd']:
    print(f'  {name}: {\"YES\" if any(name in i for i in incs) else \"MISSING\"}')"
```

Expected:

```
  LittleFS: YES
  TFT_eSPI: YES
  hardware_pwm: YES
  arduino_picocalc_kbd: YES
```

---

## Step 3: Configure CLion Project

### 3a. Open the Project

```
# From a terminal:
clion /path/to/your/project
```

Or: **File → Open** → select the project root directory (the one containing `.idea/`).

CLion should detect the `c_cpp_properties.json` and `compile_commands.json` automatically.

### 3b. Register `.ino` as C++

CLion doesn't know that `.ino` files are C++. Fix this:

1. **File → Settings → Editor → File Types**
2. Find **C/C++** in the list
3. Click **+** and add `*.ino`
4. Click **OK**

Now CLion will syntax-highlight and parse `.ino` files as C++.

### 3c. Select the IntelliSense Configuration

1. Look at the bottom-right of the CLion window — there's a configuration selector
2. It should show **ulisp-picocalc** (the name from `c_cpp_properties.json`)
3. If it shows something else (like "CLion"), click it and select **ulisp-picocalc**

### 3d. Configure Compilation Database (Optional but Recommended)

CLion can use `compile_commands.json` directly for its own analysis. This is configured in `.idea/compdb.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CompDBSettings">
    <compdbPath value="$PROJECT_DIR$/build/compile_commands.json" />
  </component>
</project>
```

The generator script should have created this. If not, create `.idea/compdb.xml` with the content above.

Then in CLion: **File → Reload CMake Project** (or **File → Invalidate Caches → Invalidate and Restart**).

### 3e. Mark Source Directories

Right-click these directories in the Project pane and select **Mark Directory as → Sources Root**:

- `ulisp-picocalc-sketch/` — the sketch source
- `ulisp-picocalc/` — the original uLisp source (read-only reference)

Mark these as **Excluded**:

- `build/` — build artifacts (the preprocessed `.cpp` is used indirectly via forced include)

---

## Step 4: Verify IntelliSense Works

Open `ulisp-picocalc-sketch/ulisp-picocalc-sketch.ino` and test:

### Test 1: Library Header Resolution

Find `#include <LittleFS.h>` (around line 42). **Ctrl+Click** on `LittleFS.h`.

✅ Should jump to `~/.arduino15/packages/rp2040/.../libraries/LittleFS/src/LittleFS.h`

### Test 2: Hardware Header Resolution

Find `#include "hardware/pwm.h"` (around line 66). **Ctrl+Click** on `hardware/pwm.h`.

✅ Should jump to `~/.arduino15/packages/rp2040/.../pico-sdk/src/rp2_common/hardware_pwm/include/hardware/pwm.h`

### Test 3: Forward-Referenced Functions

Find `pserial('[')` (around line 293). **Ctrl+Click** on `pserial`.

✅ Should jump to the forward declaration in `_ulisp_fwd_decls.h`, and from there you can navigate to the definition at line 6976.

### Test 4: Arduino Core Types

Find `TFT_eSPI tft = TFT_eSPI(320,320);` (around line 63). **Ctrl+Click** on `TFT_eSPI`.

✅ Should jump to `~/Arduino/libraries/TFT_eSPI/TFT_eSPI.h`

### If Something Doesn't Resolve

1. **File → Invalidate Caches → Just Restart** — CLion caches aggressively
2. Check the configuration name in the bottom-right selector
3. Verify `c_cpp_properties.json` has the right paths:
   ```
   python3 -c "import json; print(json.load(open('.idea/c_cpp_properties.json'))['configurations'][0]['name'])"
   ```
4. Re-run the generator script

---

## Step 5: Set Up Build Integration

### 5a. External Tool for Compilation

**File → Settings → Tools → External Tools → Click +**

- **Name:** `Build uLisp PicoCalc`
- **Program:** `bash`
- **Arguments:**
  ```
  /absolute/path/to/ttmp/.../scripts/01-compile-ulisp-picocalc.sh
  ```
- **Working directory:** `$ProjectFileDir$`

Now you can build via **Tools → External Tools → Build uLisp PicoCalc**.

### 5b. Assign a Keyboard Shortcut

**File → Settings → Keymap** → search for "Build uLisp PicoCalc" → assign **F9** or your preferred key.

### 5c. File Watcher for Auto-Regeneration (Optional)

If you want the `c_cpp_properties.json` to regenerate whenever `compile_commands.json` changes:

**File → Settings → Tools → File Watchers → Click +**

- **Name:** `Regenerate c_cpp_properties`
- **File type:** Any
- **Scope:** Project Files
- **Program:** `python3`
- **Arguments:**
  ```
  /absolute/path/to/ttmp/.../scripts/03-generate-c_cpp_properties.py
  ```
- **Working directory:** `$ProjectFileDir$`
- **Advanced → Watch path:** `build/compile_commands.json`

---

## Step 6: Suppress Expected Warnings

The uLisp codebase intentionally uses patterns that CLion flags. Suppress these to reduce noise:

### 6a. Inspection Profiles

**File → Settings → Editor → Inspections → C/C++**

Disable or set to "No highlighting" for these inspections:

| Inspection | Why Disable |
|-----------|-------------|
| **Unused declaration** | Arduino `.ino` exports functions called by the Arduino runtime |
| **C-style cast** | uLisp uses C-style casts extensively (it's C++ but written in C style) |
| **Unused parameter** | Many function signatures match Arduino callback conventions |
| **Implicit int type** | Arduino compat layer |
| **Missing include guard** | The `.ino` has no include guard by design |

### 6b. Spell-Checking

uLisp uses domain-specific terms (`pfun_t`, `BACKTRACESIZE`, `setjmp`, `longjmp`) that trigger spell-checker noise:

**File → Settings → Editor → Inspections → Proofreading → Typo** → uncheck C/C++ files.

Or add a project dictionary at `.idea/dictionaries/project.dic` with common uLisp terms.

---

## The Regeneration Workflow

The configuration is derived from the build — it's not hand-maintained. Here's the cycle:

```
┌─────────────────────────────┐
│  Edit .ino source code      │
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Run 01-compile-*.sh        │  ← produces build/compile_commands.json
│  (arduino-cli compile)      │     and build/sketch/*.ino.cpp (preprocessed)
└──────────┬──────────────────┘
           │
           ▼
┌─────────────────────────────┐
│  Run 03-generate-*.py       │  ← reads compile_commands.json
│  (generate config)          │     writes .idea/c_cpp_properties.json
└──────────┬──────────────────┘     and _ulisp_fwd_decls.h
           │
           ▼
┌─────────────────────────────┐
│  CLion reloads config       │  ← File → Reload CMake Project
│  (or auto-detects change)   │     or restart
└─────────────────────────────┘
```

**When you must regenerate:**

| Event | What to run |
|-------|-------------|
| Updated `arduino-pico` core version | Both scripts |
| Added/removed a library | Both scripts |
| Changed board FQBN | Both scripts |
| Added a new function to the `.ino` | Only `03-generate-*.py` (extracts updated forward declarations) |
| Changed source code (no new functions) | Nothing — IntelliSense doesn't need a rebuild |

---

## Troubleshooting

### "Cannot resolve" on everything

**Cause:** CLion is not using the `c_cpp_properties.json` configuration.

**Fix:** Check the bottom-right config selector. It must show `ulisp-picocalc`. If it shows `CLion` or is blank, the file isn't being picked up. Try **File → Reload CMake Project** or restart CLion.

### "Cannot resolve" on `pserial`, `pln`, `printstring`, `indent`

**Cause:** The forward declarations header is missing or stale.

**Fix:**
```
python3 ttmp/.../scripts/03-generate-c_cpp_properties.py
# Verify:
grep 'void pserial' _ulisp_fwd_decls.h
```

Then **File → Invalidate Caches → Just Restart**.

### `Arduino.h` not found

**Cause:** The Arduino core include path is wrong or the core isn't installed.

**Fix:**
```
ls ~/.arduino15/packages/rp2040/hardware/rp2040/*/cores/rp2040/Arduino.h
```
If empty, install the core: `arduino-cli core install rp2040:rp2040`

### `TFT_eSPI.h` not found

**Cause:** TFT_eSPI library isn't installed or the `User_Setup_Select.h` isn't patched.

**Fix:**
```
ls ~/Arduino/libraries/TFT_eSPI/TFT_eSPI.h
grep 'Setup60_RP2040_ILI9488' ~/Arduino/libraries/TFT_eSPI/User_Setup_Select.h
```
Both must exist. See the [build guide](02-building-ulisp-picocalc-guide.md) for TFT_eSPI patching.

### `PCKeyboard.h` not found

**Cause:** The `arduino_picocalc_kbd` library isn't installed.

**Fix:**
```
git clone https://github.com/cuu/arduino_picocalc_kbd ~/Arduino/libraries/arduino_picocalc_kbd
```

### `hardware/pwm.h` not found

**Cause:** The pico-sdk include directories weren't discovered.

**Fix:** The generator script walks `pico-sdk/src/` for all `include` directories. If the core version changed, the path in the script's output will be stale. Re-run the generator.

### IntelliSense is slow

**Cause:** 181 include paths is a lot for the indexing engine.

**Fix:** This is a one-time cost. After initial indexing completes (watch the progress bar at the bottom), navigation should be instant. If it's still slow, consider reducing the include paths by removing pico-sdk subdirectories you don't use (lwip, bluetooth, etc.).

### Changes to `.ino` aren't picked up

**Fix:** CLion should auto-detect file changes. If not, **File → Reload CMake Project** or switch away from the file and back.

---

## File Map

Quick reference of all the files involved in the CLion setup:

```
project-root/
├── .idea/
│   ├── c_cpp_properties.json        ← GENERATED: 181 includes, 105 defines, forced include
│   ├── compdb.xml                    ← Points to build/compile_commands.json
│   ├── 2026-05-05--ulisp-picocalc.iml  ← Module descriptor (CPP_MODULE)
│   └── miscellaneous.xml             ← Build settings
│
├── build/
│   ├── compile_commands.json         ← FROM BUILD: exact compiler flags per file
│   └── sketch/
│       └── ulisp-picocalc-sketch.ino.cpp  ← FROM BUILD: Arduino-preprocessed source
│
├── _ulisp_fwd_decls.h               ← GENERATED: 36 minimal forward declarations
│
├── ulisp-picocalc-sketch/
│   └── ulisp-picocalc-sketch.ino     ← THE SOURCE: what you edit
│
├── ulisp-picocalc/
│   ├── ulisp-picocalc.ino            ← ORIGINAL: technoblogy source (reference)
│   └── Setup60_RP2040_ILI9488.h      ← Display config for TFT_eSPI
│
└── ttmp/.../scripts/
    ├── 01-compile-ulisp-picocalc.sh        ← Build script
    ├── 03-generate-c_cpp_properties.py     ← Config generator (run after build)
    └── 04-extract-forward-declarations.py  ← Standalone fwd-decl extractor
```

---

## Adapting This for Other Arduino Projects

This playbook uses uLisp PicoCalc as the worked example, but the approach is general. To adapt for any Arduino/RP2040 project:

1. **Compile once** with `arduino-cli compile --fqbn <your-fqbn> --build-path ./build`
2. **Verify** `build/compile_commands.json` exists
3. **Run** `03-generate-c_cpp_properties.py --project-dir /path/to/project`
4. **Open** in CLion, register `.ino` as C++, select the config
5. **Done**

The generator script is project-agnostic — it reads whatever `compile_commands.json` it finds. It works for ESP32, STM32, AVR, or any other Arduino core, as long as `arduino-cli` produced the compilation database.

---

## Appendix: The Forward Declaration Problem in Detail

This section explains *why* the forward declarations matter and *how* Arduino handles them, for anyone debugging a similar setup.

### What Arduino Does to `.ino` Files

The Arduino builder (used by both the Arduino IDE and `arduino-cli`) performs three transformations on `.ino` files before passing them to the C++ compiler:

1. **Prepend `#include <Arduino.h>`** — makes all Arduino types and functions available
2. **Scan for function signatures** — a simple regex-based scan that finds lines matching `type name(params) {`
3. **Insert forward declarations** — for every function found, a prototype is generated and inserted before the first function body

For uLisp PicoCalc, this produces **462 forward declarations** spanning ~925 lines. But only **36** of those are actually needed — functions that are called before their definition in the file. The generator script (`03-generate-c_cpp_properties.py`) analyzes the `.ino` to find which functions are forward-referenced and only includes those 36 in `_ulisp_fwd_decls.h`.

### Why This Breaks CLion

CLion's IntelliSense engine parses `.ino` files directly as C++. It doesn't run the Arduino preprocessor. So it sees:

```cpp
// line 293
pserial('[');       // ERROR: pserial not declared

// ... 6,700 lines later ...

// line 6976
void pserial (char c) {
    if (pfun == pserial) Serial.write(c);
    else pfun(c);
}
```

### The Fix: `forcedInclude`

The `c_cpp_properties.json` standard supports a `forcedInclude` field — a list of headers that are automatically `#include`d before every file is parsed. We extract the 36 needed prototypes into `_ulisp_fwd_decls.h` and list it as a forced include. CLion then behaves as if the `.ino` file starts with those 36 declarations.

### What Could Go Wrong

- **Stale forward declarations:** If you add a new function to the `.ino` and forget to regenerate, the new function won't have a forward declaration. CLion will show "cannot resolve" for calls to it. Fix: re-run the generator.
- **Signature mismatch:** If you change a function's signature (parameter types, return type), the old forward declaration may conflict. Fix: recompile (which updates the preprocessed `.cpp`), then re-run the generator.
- **Inline functions:** Some functions like `pln` are declared `inline`. The forward declaration from Arduino has the correct signature but without `inline`. This works for IntelliSense (symbol resolution) but would fail at link time if the declaration were actually compiled. Since we're only using it for IDE navigation, not compilation, this is fine.
