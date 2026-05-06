---
Title: Integration Implementation Diary
Ticket: repl-window
Status: active
Topics:
    - picocalc
    - repl
    - display
    - keyboard
    - editor
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/05-integrating-repl-window-header-into-ulisp-picocalc-ino.md
      Note: Design guide for integration work
    - Path: ulisp-picocalc/repl_window.h
      Note: Created in nested commit 84a1e97
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Modified in nested commit 84a1e97
ExternalSources: []
Summary: Diary for integrating the primitive REPL window into the real uLisp PicoCalc firmware.
LastUpdated: 2026-05-06T03:55:00-04:00
WhatFor: Record integration steps, compile results, and follow-up risks
WhenToUse: When resuming or reviewing the uLisp REPL-window integration
---


# Diary

## Goal

Record the work to extract the primitive REPL window into a header and wire it into the real `ulisp-picocalc.ino` firmware.

## Step 1: Header extraction and first uLisp integration compile

I extracted the dirty-cell REPL renderer, transcript buffer, edit buffer, and keyboard input handling into `ulisp-picocalc/repl_window.h`, then wired it into the real uLisp firmware. The real REPL now routes `pserial()` through the back buffer and routes PicoCalc keyboard input through the edit buffer/committed-input model used by the primitive sketch.

I intentionally removed the feature flag/backwards path after the user asked to simplify the integration. The old terminal functions remain present in the file, but normal REPL output no longer calls `Display(c)` from `pserial()`.

### Prompt Context

**User prompt (verbatim):** "Ok cool, let's commit. Let's make a new version where we make this a header file to include, and we include it in the normal ulisp picocalc ino, and use the new REPL (no gfx commands, we don't need them for now) to do input editing, and output printing. 

Create a design and implementation guide, then tasks. then implement the tasks and commit at appropriate intervals. Keep a diary as you work."

**Assistant interpretation:** Extract the proven primitive REPL-window code into a reusable header, include it in the real uLisp PicoCalc firmware, replace the real REPL input/output paths, document the design, add tasks, compile, and commit in steps.

**Inferred user intent:** Move from a standalone hardware primitive experiment to the first real uLisp integration while keeping the implementation reviewable and documented.

**Commit (code):** 84a1e97d8d1ddce56399289f9de51d7a9f4a7cf9 — "Integrate REPL window header"

### What I did

- Created design doc:
  - `design/05-integrating-repl-window-header-into-ulisp-picocalc-ino.md`
- Created header:
  - `ulisp-picocalc/repl_window.h`
- Header includes:
  - geometry constants,
  - 16-color palette,
  - packed fg/bg attributes,
  - `ReplBackBuffer`,
  - `ReplEditBuffer`,
  - dirty-cell renderer,
  - `ReplWindowInit()`,
  - `ReplBackBufferAppend()`,
  - `ReplProcessKey()`,
  - `ReplRenderIfDirty()`.
- Modified `ulisp-picocalc/ulisp-picocalc.ino`:
  - included `repl_window.h`,
  - changed `pserial()` to call `ReplBackBufferAppend(c)`,
  - changed `gserial()` to poll the PicoCalc keyboard into `ReplProcessKey()` and return committed edit-buffer chars,
  - added `ReplWindowInit()` after `initgfx()`,
  - added `ReplRenderIfDirty()` after the REPL prints evaluation output.
- Renamed `ulisp-picocalc-comments.ino` to `ulisp-picocalc-comments.ino.bak` because Arduino compiles all `.ino` files in a sketch directory. Keeping a second full `.ino` in the folder caused duplicate definitions.
- Compiled from repo root with the sketch folder as target:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-ulisp-integrate \
  --warnings all \
  ulisp-picocalc
```

### Why

`repl-window-primitives` proved the display and keyboard model in isolation. The next required step is to make uLisp's existing `repl()` use the same model without rewriting the reader/evaluator/printer.

### What worked

The integrated sketch compiles successfully after fixing two real issues:

1. `PCKeyboard.h` has no include guard, so `repl_window.h` must not include it if `ulisp-picocalc.ino` already does.
2. Arduino's sketch model compiles every `.ino` in the sketch folder, so `ulisp-picocalc-comments.ino` had to stop being a `.ino` file.

Compile result:

```text
Sketch uses 200356 bytes (9%) of program storage space. Maximum is 2093056 bytes.
Global variables use 227760 bytes (86%) of dynamic memory, leaving 34384 bytes for local variables. Maximum is 262144 bytes.
Low memory available, stability problems may occur.
```

### What didn't work

The first compile attempt failed with duplicate definitions because both `ulisp-picocalc.ino` and `ulisp-picocalc-comments.ino` were compiled together:

```text
error: redefinition of 'void setup()'
error: redefinition of 'void repl(object*)'
error: redefinition of 'void loop()'
```

After renaming the comments file, the next compile failed because `repl_window.h` included `PCKeyboard.h`, and that header lacks include guards:

```text
error: redefinition of 'class PCKeyboard'
```

Finally, one typo in the header called `ReplBackBufferAppend("0x")` even though the function takes a single `char`. I replaced the unknown-key diagnostic with `[?]` for v1.

### What I learned

Arduino sketch structure matters: the compile unit is the sketch directory, not one selected `.ino` file. All `.ino` files in the directory are concatenated/compiled. This is why the comments copy caused duplicate definitions.

Also, for project-local headers included from `.ino`, avoid including third-party headers without include guards if the main sketch already included them.

### What was tricky to build

The hardest part was not the REPL integration itself; it was making the Arduino sketch model explicit. The directory had a full commented copy of the firmware with `.ino` extension, which made the folder invalid as a single Arduino sketch. Renaming it to `.ino.bak` makes the build model sane.

RAM is now the main technical risk. The integrated header keeps 160 transcript rows plus attrs/bold and two render buffers. That is acceptable for a first compile, but likely too high for stable uLisp runtime.

### What warrants a second pair of eyes

- Global RAM usage: 227,760 bytes / 262,144 bytes is high.
- Whether the full 160-row back buffer is too large for the final firmware.
- Whether `gserial()` should preserve serial monitor input exactly as currently written.
- Whether old `Display()` users (especially editor/special terminal features) still behave acceptably.

### What should be done in the future

- Reduce memory before hardware testing if stability is poor:
  - reduce `ReplBackBufferRows`,
  - remove per-cell `bold[][]`,
  - maybe compose rows one at a time instead of storing `replDesired`.
- Hardware-test boot, typing, Enter commit, evaluation output, errors, and Escape.
- Revisit autocomplete and parenthesis highlighting.

### Code review instructions

Start with:

```text
ulisp-picocalc/repl_window.h
```

Then review changed integration points in:

```text
ulisp-picocalc/ulisp-picocalc.ino
```

Key functions:

```text
pserial
gserial
setup
repl
```

Validate with:

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico --build-path build-ulisp-integrate --warnings all ulisp-picocalc
```

### Technical details

The successful compile emitted expected warnings from uLisp longjmp paths and TFT_eSPI touch support, plus the low-memory warning quoted above.

## Step 2: Shrink uLisp heap and upload integrated REPL-window firmware

After the first integrated compile showed high global RAM usage, the user clarified that we should simply reduce the uLisp heap. I reduced the RP2040 `WORKSPACESIZE` from `(23000-SDSIZE)` to `(18000-SDSIZE)`, which trades Lisp heap capacity for runtime headroom needed by the REPL window buffers.

The reduced-heap integrated firmware compiled successfully and was uploaded to the PicoCalc SD card under `/pico1-apps` with distinct filenames so it does not overwrite the primitive experiment build.

### Prompt Context

**User prompt (verbatim):** "wait no, just make the lisp heap smaller" followed by "compile and upload to sd card" and "continue"

**Assistant interpretation:** Reduce uLisp workspace RAM instead of optimizing the REPL window buffers, then build and upload the integrated firmware artifacts to the PicoCalc SD-card app folder.

**Inferred user intent:** Quickly get a testable integrated uLisp REPL-window firmware with enough remaining RAM to be more stable on hardware.

**Commit (code):** 84a1e97d8d1ddce56399289f9de51d7a9f4a7cf9 — "Integrate REPL window header"

### What I did

- Changed the RP2040 heap definition in `ulisp-picocalc/ulisp-picocalc.ino`:

```cpp
#define WORKSPACESIZE (18000-SDSIZE)    /* Objects (8*bytes) — reduced to leave RAM for REPL window */
```

- Recompiled the integrated sketch:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-ulisp-integrate \
  --warnings all \
  ulisp-picocalc
```

- Uploaded artifacts to SD card:

```text
/Volumes/NO NAME/pico1-apps/PicoCalc_uLisp_REPL_Window.uf2
/Volumes/NO NAME/pico1-apps/uLisp_REPL_Window.bin
```

### Why

The first integrated compile used 227,760 bytes of globals, leaving only 34,384 bytes. Reducing `WORKSPACESIZE` by 5,000 objects frees approximately 40,000 bytes because each uLisp object is 8 bytes.

### What worked

Compile succeeded with substantially better RAM headroom:

```text
Sketch uses 200348 bytes (9%) of program storage space. Maximum is 2093056 bytes.
Global variables use 187760 bytes (71%) of dynamic memory, leaving 74384 bytes for local variables. Maximum is 262144 bytes.
```

Remote upload verification showed:

```text
/Volumes/NO NAME/pico1-apps/PicoCalc_uLisp_REPL_Window.uf2  438272 bytes
/Volumes/NO NAME/pico1-apps/uLisp_REPL_Window.bin            219024 bytes
```

### What didn't work

No compile or upload failure occurred in this step. Existing warnings remain:

- `TFT_eSPI` touch warning (`TOUCH_CS` not defined), expected.
- uLisp `longjmp` clobber warnings, pre-existing.

### What I learned

The heap reduction did exactly what expected: 5,000 fewer objects × 8 bytes/object = ~40 KB RAM freed. That moved globals from 86% to 71% RAM use.

### What was tricky to build

The main tricky part is choosing a heap size that still leaves uLisp useful. `(18000-SDSIZE)` is a pragmatic first hardware-test value, not a final tuned value.

### What warrants a second pair of eyes

- Whether 18,000 objects is enough for realistic PicoCalc uLisp sessions.
- Whether the REPL window buffers should still be optimized after hardware testing.
- Whether the renamed `ulisp-picocalc-comments.ino.bak` should be moved out of the sketch directory entirely.

### What should be done in the future

- Hardware-test `PicoCalc_uLisp_REPL_Window.uf2` from `/pico1-apps`.
- Verify boot, prompt, input editing, evaluation, output, error recovery.
- Tune `WORKSPACESIZE` upward/downward based on real behavior.

### Code review instructions

Review:

```text
ulisp-picocalc/ulisp-picocalc.ino
ulisp-picocalc/repl_window.h
```

Validate with:

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico --build-path build-ulisp-integrate --warnings all ulisp-picocalc
```

### Technical details

Uploaded artifacts:

```text
build-ulisp-integrate/ulisp-picocalc.ino.uf2 → /pico1-apps/PicoCalc_uLisp_REPL_Window.uf2
build-ulisp-integrate/ulisp-picocalc.ino.bin → /pico1-apps/uLisp_REPL_Window.bin
```
