---
Title: Primitive Experiment Implementation Diary
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
    - Path: repl-window-primitives/repl-window-primitives.ino
      Note: Code created and compiled in diary Step 1
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md
      Note: Design document created in diary Step 1
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/03-primitive-picocalc-experiment-implementation-guide.md
      Note: Implementation guide created in diary Step 1
ExternalSources: []
Summary: Chronological diary for the standalone C++ PicoCalc primitive display/keyboard experiment.
LastUpdated: 2026-05-06T02:58:00-04:00
WhatFor: Record decisions, commands, failures, commits, and validation for the primitive experiment
WhenToUse: When resuming work on repl-window-primitives or reviewing implementation history
---


# Diary

## Goal

This diary records the implementation journey for the standalone C++ PicoCalc primitive experiment. The experiment exists to validate screen drawing, keyboard events, edit-buffer behavior, and back-buffer rendering before porting the ideas into uLisp.

## Step 1: Create the primitive sketch and planning documents

I created a new standalone Arduino sketch named `repl-window-primitives/repl-window-primitives.ino`. The sketch initializes the PicoCalc TFT and keyboard, maintains an append-only transcript/back buffer, maintains a mutable edit buffer, renders transcript/status/input areas, and logs special key events so hardware testing can discover real key codes.

I also created a separate design document and implementation guide under the existing `repl-window` docmgr ticket. These documents explain why the primitive sketch exists, how it differs from the uLisp integration work, what primitives it validates, and how future work should proceed.

### Prompt Context

**User prompt (verbatim):** "Create a separate, C++ only .ino where we experiment with drawing to the screen and doing keyboard. That way we can work with primitives only before adding ulisp. 

Create a separate design and implementation document, add tasks, and upload the design document to remarkable. Then and then work on them one by one, committing at appropriate intervals, keeping a detailed diary."

**Assistant interpretation:** Create a standalone Arduino sketch for PicoCalc display/keyboard experiments, document its design and implementation, add milestone tasks, upload the design doc to reMarkable, and proceed in small committed steps with a diary.

**Inferred user intent:** De-risk the eventual uLisp REPL window by validating low-level PicoCalc UI primitives in isolation before modifying the interpreter.

**Commit (code):** c46ef29c3d3f41772148d1aa670fb78e208f270c — "Add PicoCalc REPL primitive sketch"

### What I did

- Created `repl-window-primitives/repl-window-primitives.ino`.
- Added a C++-only Arduino sketch using:
  - `TFT_eSPI`,
  - `PCKeyboard`,
  - `Wire1` on `SDA=6`, `SCL=7`,
  - `pc_kbd.begin(0x1f, &Wire1)`.
- Implemented a physical-row transcript ring buffer:
  - `BackBufferState`,
  - `appendBackChar`,
  - `appendBackString`,
  - `appendBackLine`,
  - row wrapping and tail-follow behavior.
- Implemented a mutable input line:
  - `EditBufferState`,
  - insert,
  - backspace,
  - delete,
  - cursor left/right,
  - commit.
- Implemented full-screen rendering:
  - transcript viewport,
  - status row,
  - input line with cursor.
- Implemented keyboard polling and tentative special-key mappings:
  - Enter commits,
  - Backspace edits,
  - candidate arrows/Delete are mapped/logged,
  - unknown non-printable keys are logged.
- Created design document:
  - `design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md`.
- Created implementation guide:
  - `reference/03-primitive-picocalc-experiment-implementation-guide.md`.
- Created this diary:
  - `reference/02-primitive-experiment-implementation-diary.md`.
- Compiled with:

```bash
cd /home/manuel/code/wesen/2026-05-05--ulisp-picocalc
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

### Why

The uLisp firmware mixes interpreter logic and PicoCalc UI logic in one large `.ino`. A primitive sketch gives us a safe place to prove screen and keyboard behavior before changing `pserial()`, `gserial()`, `ProcessKey()`, or `Display()` inside the interpreter firmware.

### What worked

- The standalone sketch compiled successfully.
- The compile used the installed `PCKeyboard` and `TFT_eSPI` libraries.
- Memory use is modest for the experimental buffers:
  - program storage: 77,664 bytes,
  - globals: 19,888 bytes,
  - remaining RAM: 242,256 bytes.

### What didn't work

- No blocking compile failures occurred.
- The compile emitted the expected `TFT_eSPI` warning:

```text
/home/manuel/Arduino/libraries/TFT_eSPI/TFT_eSPI.h:973:8: warning: #warning >>>>------>> TOUCH_CS pin not defined, TFT_eSPI touch functions will not be available! [-Wcpp]
```

This is expected because the sketch does not use touch.

### What I learned

The primitive architecture can compile independently from uLisp while still using the same hardware libraries and core PicoCalc initialization strategy. A physical-row back buffer is simple and cheap enough for experimentation: `160 * 53 = 8,480` bytes before metadata.

### What was tricky to build

The biggest design choice was whether to model transcript entries as logical lines or physical screen rows. I chose physical rows for the experiment because it keeps wrapping and rendering obvious, and it maps directly to the PicoCalc character grid. This is not necessarily the final uLisp design, but it is the lowest-risk primitive to validate on hardware.

Special key codes are still uncertain. The existing uLisp code filters `0xA1` through `0xA5`, so the primitive sketch treats those as candidate navigation/delete keys and logs behavior. Hardware testing must confirm or correct these mappings.

### What warrants a second pair of eyes

- Whether full-screen redraw every 20 ms is acceptable on the PicoCalc TFT.
- Whether tentative key mappings match the actual keyboard firmware.
- Whether `Wire1.setClock(10000)` is necessary or too conservative for this standalone sketch.
- Whether `BackBufferRows = 160` is the right default for the eventual uLisp integration.

### What should be done in the future

- Flash the sketch on PicoCalc hardware.
- Record real key codes for arrows, Delete, Home/End, Tab, and Escape.
- Add dirty-row rendering if full redraw flickers.
- Add input history and scrollback controls once key codes are confirmed.
- Port the validated primitives back into `ulisp-picocalc.ino`.

### Code review instructions

Start with:

```text
repl-window-primitives/repl-window-primitives.ino
```

Review in this order:

1. display geometry constants,
2. `BackBufferState` and append functions,
3. `EditBufferState` and edit functions,
4. renderer functions,
5. keyboard processing,
6. `setup()` and `loop()`.

Validate with:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

### Technical details

Compile result:

```text
Sketch uses 77664 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 19888 bytes (7%) of dynamic memory, leaving 242256 bytes for local variables. Maximum is 262144 bytes.
```

## Step 2: Add fake evaluator commands for scroll and wrapping stress tests

I added a tiny fake evaluator to the primitive sketch. This keeps the sketch uLisp-free while allowing us to exercise evaluator-like output paths: normal text now produces `fake-eval: ...`, `/help` prints command help, `/status` prints buffer state, `/clear` clears the transcript, and `/spam` prints many deterministic long lines.

This step is useful because the back buffer and renderer need to handle output bursts, line wrapping, and row rollover before the same primitives are ported to `pserial()` in the uLisp firmware.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Continue working through primitive-sketch tasks one at a time, committing focused increments and recording the work.

**Inferred user intent:** Build confidence in the primitive UI model through small, reviewable steps before uLisp integration.

**Commit (code):** f9ae1f2028ff8dda3b8f1036dbd1573ada5f55c2 — "Add primitive fake evaluator commands"

### What I did

- Added `clearBackBuffer()`.
- Added `runFakeEvaluator(const char *input)`.
- Changed `commitEditBuffer()` so it snapshots the committed input, appends the prompt/input line, runs fake evaluation, and then resets the edit buffer.
- Added commands:
  - `/help`,
  - `/spam`,
  - `/clear`,
  - `/status`.
- Updated startup text to advertise the commands.
- Updated primitive tasks to mark fake evaluator mode complete.
- Updated the primitive implementation guide with command references.
- Recompiled with:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

### Why

The renderer needs deterministic stress output before hardware testing and before uLisp integration. `/spam` gives a repeatable way to generate enough text to test wrapping, scrollback, viewport following, and back-buffer rollover.

### What worked

- The sketch still compiles successfully.
- Program storage changed from 77,664 bytes to 78,224 bytes.
- Global RAM use remained 19,888 bytes.

### What didn't work

- No compile failures occurred.
- The same expected `TFT_eSPI` touch warning remains:

```text
warning: #warning >>>>------>> TOUCH_CS pin not defined, TFT_eSPI touch functions will not be available! [-Wcpp]
```

### What I learned

A fake evaluator is enough to test the intended `pserial()` output model without embedding uLisp. The experiment can now produce short output, status output, clear behavior, and long deterministic output from inside the primitive UI shell.

### What was tricky to build

`commitEditBuffer()` must copy the input before running commands because `/clear` and future commands can mutate transcript state while the edit buffer is about to reset. I used a local `committed[InputBufferSize]` snapshot before appending and evaluating.

### What warrants a second pair of eyes

- Whether `/clear` should preserve startup/help lines or fully reset the transcript as it does now.
- Whether the fake evaluator should eventually support scripted output timings, not just immediate bursts.

### What should be done in the future

- Test `/spam` on hardware to evaluate redraw flicker.
- Add dirty-row rendering if `/spam` exposes slow full-screen redraws.

### Code review instructions

Review these symbols:

```text
clearBackBuffer
runFakeEvaluator
commitEditBuffer
```

Validate with the same `arduino-cli compile` command and, on hardware, type `/help`, `/status`, `/spam`, and `/clear`.

### Technical details

Compile result:

```text
Sketch uses 78224 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 19888 bytes (7%) of dynamic memory, leaving 242256 bytes for local variables. Maximum is 262144 bytes.
```
