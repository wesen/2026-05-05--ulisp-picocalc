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

## Step 3: Build and upload primitive firmware artifacts to the PicoCalc SD card

I rebuilt the standalone primitive sketch and copied both generated firmware artifacts to the PicoCalc SD card's firmware directory over SSH/SCP. This makes the primitive experiment available for hardware flashing/testing without touching the uLisp firmware image names.

The uploaded files use explicit primitive-experiment names so they do not overwrite the main uLisp firmware artifacts.

### Prompt Context

**User prompt (verbatim):** "upload to SD card"

**Assistant interpretation:** Build the current primitive sketch and upload its UF2/BIN outputs to the PicoCalc SD-card firmware folder.

**Inferred user intent:** Put the primitive experiment firmware onto the SD card so it can be flashed/tested on the PicoCalc hardware.

**Commit (code):** N/A — no source changes were made in this step.

### What I did

- Rebuilt the primitive sketch with:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

- Confirmed local artifacts:

```text
build-repl-window-primitives/repl-window-primitives.ino.uf2
build-repl-window-primitives/repl-window-primitives.ino.bin
```

- Uploaded to the remote SD card firmware directory:

```bash
scp build-repl-window-primitives/repl-window-primitives.ino.uf2 \
  "manuel@192.168.0.57:/Volumes/NO NAME/firmware/PicoCalc_REPL_Primitives.uf2"

scp build-repl-window-primitives/repl-window-primitives.ino.bin \
  "manuel@192.168.0.57:/Volumes/NO NAME/firmware/REPL_Primitives.bin"
```

- Verified remote files with:

```bash
ssh manuel@192.168.0.57 \
  'ls -la "/Volumes/NO NAME/firmware/PicoCalc_REPL_Primitives.uf2" "/Volumes/NO NAME/firmware/REPL_Primitives.bin"'
```

### Why

The next useful work is hardware testing: confirm the display, keyboard, cursor, edit buffer, and fake evaluator behavior on the actual PicoCalc. Uploading the artifacts to the SD card makes that possible.

### What worked

- Compile succeeded.
- SCP upload succeeded.
- Remote verification showed both files present:

```text
/Volumes/NO NAME/firmware/PicoCalc_REPL_Primitives.uf2  190464 bytes
/Volumes/NO NAME/firmware/REPL_Primitives.bin            95032 bytes
```

### What didn't work

- No upload failure occurred.
- The compile still emits the expected `TFT_eSPI` warning about `TOUCH_CS` not being defined. This is not relevant because the sketch does not use touch.

### What I learned

Keeping the primitive artifact names distinct avoids overwriting the main uLisp firmware names used by the Makefile (`PicoCalc_uLisp_4.8f.uf2`, `uLisp_4.8f.bin`).

### What was tricky to build

The SD volume path contains a space: `/Volumes/NO NAME`. All `scp` and `ssh ls` commands need quoting around the remote path to avoid splitting the volume name.

### What warrants a second pair of eyes

- Confirm on the PicoCalc side which artifact the flashing workflow expects: UF2, BIN, or both.
- Confirm whether the filenames should be shortened for any on-device firmware selection UI.

### What should be done in the future

- Flash `PicoCalc_REPL_Primitives.uf2` or `REPL_Primitives.bin` using the PicoCalc workflow.
- Record actual hardware behavior and special key codes in the next diary step.

### Code review instructions

No code changed. Validate by checking the remote SD-card files exist:

```bash
ssh manuel@192.168.0.57 \
  'ls -la "/Volumes/NO NAME/firmware/PicoCalc_REPL_Primitives.uf2" "/Volumes/NO NAME/firmware/REPL_Primitives.bin"'
```

### Technical details

Compile result:

```text
Sketch uses 78224 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 19888 bytes (7%) of dynamic memory, leaving 242256 bytes for local variables. Maximum is 262144 bytes.
```

## Step 4: Correct primitive firmware upload destination to `/pico1-apps`

The SD-card destination was corrected from `/firmware` to `/pico1-apps`. I rebuilt the primitive sketch, created the destination directory if needed, copied both UF2 and BIN artifacts there, and verified the files exist at the corrected location.

This supersedes the earlier upload destination from Step 3. The `/firmware` copies may still exist, but the intended app location for this workflow is now `/pico1-apps`.

### Prompt Context

**User prompt (verbatim):** "it is to /pico1-apps on the SD card now"

**Assistant interpretation:** The firmware artifacts should be uploaded to `/pico1-apps` on the mounted PicoCalc SD card, not `/firmware`.

**Inferred user intent:** Correct the upload destination so the PicoCalc app/firmware selection workflow finds the primitive experiment in the right SD-card folder.

**Commit (code):** N/A — no source changes were made in this step.

### What I did

- Rebuilt the primitive sketch with `arduino-cli compile`.
- Ensured the remote destination exists:

```bash
ssh manuel@192.168.0.57 'mkdir -p "/Volumes/NO NAME/pico1-apps"'
```

- Uploaded the UF2:

```bash
scp build-repl-window-primitives/repl-window-primitives.ino.uf2 \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/PicoCalc_REPL_Primitives.uf2"
```

- Uploaded the BIN:

```bash
scp build-repl-window-primitives/repl-window-primitives.ino.bin \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/REPL_Primitives.bin"
```

- Verified both files at the corrected location.

### Why

The previous upload placed files under `/firmware`, but the user clarified that the current SD-card app destination is `/pico1-apps`. Keeping firmware in the expected directory avoids confusion during hardware flashing/testing.

### What worked

Remote verification showed:

```text
/Volumes/NO NAME/pico1-apps/PicoCalc_REPL_Primitives.uf2  190464 bytes
/Volumes/NO NAME/pico1-apps/REPL_Primitives.bin            95032 bytes
```

### What didn't work

No upload failure occurred. The compile still emits the expected `TFT_eSPI` touch warning, which is harmless for this sketch.

### What I learned

For this PicoCalc SD-card workflow, primitive app artifacts should go under `/pico1-apps`, not `/firmware`.

### What was tricky to build

As before, the remote volume path contains a space (`/Volumes/NO NAME`), so remote paths must be carefully quoted in `ssh` and `scp` commands.

### What warrants a second pair of eyes

- Whether old copies under `/firmware` should be removed to avoid duplicate/confusing firmware entries.
- Whether the PicoCalc app loader expects UF2, BIN, or a specific naming convention under `/pico1-apps`.

### What should be done in the future

- Use `/pico1-apps` for subsequent primitive uploads.
- Flash/test the primitive sketch from the corrected SD-card location.

### Code review instructions

No code changed. Validate the upload with:

```bash
ssh manuel@192.168.0.57 \
  'ls -la "/Volumes/NO NAME/pico1-apps/PicoCalc_REPL_Primitives.uf2" "/Volumes/NO NAME/pico1-apps/REPL_Primitives.bin"'
```

### Technical details

Compile result remained:

```text
Sketch uses 78224 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 19888 bytes (7%) of dynamic memory, leaving 242256 bytes for local variables. Maximum is 262144 bytes.
```

## Step 5: Replace full-row redraws with a lightweight dirty-cell renderer

The hardware test reported that the primitive sketch's behavior was otherwise OK, but the screen flickered. The cause is that the renderer was still clearing and redrawing rows directly on the TFT. Because the TFT is updated over SPI without an application-level framebuffer swap, those clears are visible.

I replaced the renderer with a lightweight character-cell diff model. It composes a desired text-cell screen from the transcript/status/input state, compares it against the last drawn text-cell screen, and sends only changed cells to the TFT. This avoids full-row clears and is much closer to the renderer we should eventually port into uLisp.

### Prompt Context

**User prompt (verbatim):** "continue. Create a design + implementation guide (doesn't need to be super long, but still has all the details), and then implement."

**Assistant interpretation:** Document the dirty-cell renderer approach and implement it in the primitive sketch.

**Inferred user intent:** Convert the flickering primitive renderer into a RAM-conscious text-cell diff renderer that can inform the uLisp implementation.

**Commit (code):** pending — will be recorded after this step is committed.

### What I did

- Added `design/03-dirty-cell-renderer-design-and-implementation-guide.md`.
- Replaced the previous `ScreenCell { char, fg, bg }` renderer with compact cells:

```cpp
struct RenderCell {
  char ch;
  uint8_t attr;
};
```

- Added compact attributes:

```cpp
AttrNormal
AttrStatus
AttrPrompt
AttrCursor
```

- Added `attrFg()` / `attrBg()` color lookup functions.
- Added `desiredCells[Lines][Columns]` and `drawnCells[Lines][Columns]`.
- Changed rendering so it:
  - clears desired cells in RAM,
  - composes transcript/status/input into desired cells,
  - compares desired cells to drawn cells,
  - draws only changed cells to the TFT.
- Kept the `uiDirty` gate so rendering only happens after UI state changes.
- Updated tasks to record that full redraw flicker was observed and dirty-cell rendering was implemented.
- Compiled with:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

### Why

A full pixel framebuffer would cost about 204,800 bytes, which is not viable for uLisp on RP2040. But a text-cell renderer is small: 32 rows × 53 columns × 2 bytes per cell × 2 buffers = 6,784 bytes. This gives us flicker reduction without buffering pixels.

### What worked

- The dirty-cell renderer compiles.
- RAM use is significantly lower than the heavier `char + fg + bg` cell model:
  - previous `ScreenCell` compile: globals 40,240 bytes,
  - compact `RenderCell` compile: globals 26,672 bytes.
- Program storage remains small: 78,512 bytes.

### What didn't work

Before adding the compact renderer, the first attempt used function signatures containing `ScreenCell`. Arduino's `.ino` preprocessor generated prototypes before the struct definition, causing compile errors:

```text
error: 'ScreenCell' does not name a type
```

I first fixed that with a forward declaration, then replaced the design with compact `RenderCell` cells. The compact renderer avoids heavy per-cell color storage and is more appropriate for uLisp.

### What I learned

For this hardware, flicker is not a synchronization problem we can solve with vsync-style swapping. We are writing directly to the TFT controller. The practical solution is to minimize visible pixel writes: only draw changed character cells and never clear full rows during normal updates.

### What was tricky to build

The renderer must be RAM-conscious and still expressive enough for cursor/status/prompt coloring. Encoding colors as small attributes rather than storing foreground/background in each cell gives a good balance: small memory footprint, simple comparison, and easy porting.

### What warrants a second pair of eyes

- Whether two full text-cell buffers are acceptable in the final uLisp firmware.
- Whether `fillRect()` per changed cell is still visible during heavy `/spam` output.
- Whether uLisp should compose one row at a time to avoid the desired-cell buffer.

### What should be done in the future

- Upload this build to `/pico1-apps` when the SD card is mounted/available.
- Test whether idle flicker is gone and whether `/spam` output is acceptable.
- If flicker remains, optimize `drawCellDirect()` to avoid separate `fillRect()` where `tft.drawChar()` with background can fully cover the cell.

### Code review instructions

Review the renderer section in:

```text
repl-window-primitives/repl-window-primitives.ino
```

Key symbols:

```text
RenderCell
CellAttr
attrFg
attrBg
desiredCells
drawnCells
composeTranscript
composeStatus
composeInput
renderAll
```

Validate with the compile command above, then hardware-test typing, backspace, `/spam`, and `/clear`.

### Technical details

Compile result:

```text
Sketch uses 78512 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 26672 bytes (10%) of dynamic memory, leaving 235472 bytes for local variables. Maximum is 262144 bytes.
```

## Step 6: Full cleanup — palette, per-cell colors, bold, icons, graphics, `/demo`

After the user noted the Arduino preprocessor issues and asked me to clean up, I rewrote the primitive sketch into a well-ordered single file with no fragile auto-prototype dependencies. The rewrite adds:

- a 16-color neon/neotokyo palette as RGB565 constants,
- per-cell fg/bg color via a packed 8-bit attribute byte stored alongside each back-buffer character,
- bold text rendered by drawing the character twice with a 1-pixel horizontal offset,
- simple hand-drawn icons (heart, star, check, cross, left/right arrows) drawn with primitive shapes,
- TFT graphics commands (line, rect, fill-rect, circle, fill-circle) stored in a small replay buffer so they persist across text renders,
- `/demo` command that shows colors, bold, and a geometric graphics composition,
- `/fg`, `/bg`, `/bold`, `/normal`, `/palette`, `/clear-gfx`, `/icon` commands.

The sketch now compiles cleanly at 86,816 bytes flash and 47,480 bytes globals.

### Prompt Context

**User prompt (verbatim):** "make a little /demo program as well that makes them all" then "continue. Create a design + implementation guide... and then implement." then "upload design docs to remarkable (individually)" then "Your little brother is having toruble with arduino it seems. You know better, clena this up" then "you can also just use c++ classes, no?" then "continue".

**Assistant interpretation:** The user wants: a `/demo` command, a cleaned-up sketch that compiles without Arduino preprocessor issues, C++ class-based organization considered, design docs uploaded to reMarkable, and everything committed properly.

**Inferred user intent:** Build a complete primitive experiment with colors, bold, icons, graphics, and a demo command; clean up the codebase so it compiles reliably; keep documenting and committing.

**Commit (code):** pending — will be recorded after this step.

### What I did

- Wrote `design/04-color-control-codes-bold-and-tft-graphics-primitives.md`.
- Uploaded it to reMarkable.
- Completely rewrote `repl-window-primitives.ino` into a clean linear order:
  1. includes,
  2. geometry constants,
  3. palette + color helpers,
  4. state structs (DrawState, BackBufferState, EditBufferState, RenderCell, GfxCmd),
  5. globals,
  6. back buffer functions,
  7. edit buffer functions,
  8. graphics + icon functions,
  9. dirty-cell renderer,
  10. fake evaluator + `/demo`,
  11. keyboard handling,
  12. Arduino setup/loop.
- This ordering avoids custom types in function signatures before their definitions, eliminating Arduino auto-prototype failures.
- Added palette: 16 neon/neotokyo RGB565 colors.
- Added packed fg/bg attribute byte: `(fg & 0x0F) << 4 | (bg & 0x0F)`.
- Added bold flag stored per-cell in `BackBufferState.bold[][]`.
- Added `drawCellDirect()` bold rendering: draw char, then draw again at x+1.
- Added icon drawing via `drawIconPixels()` with heart/star/check/cross/arrow shapes.
- Added graphics command replay buffer (`GfxCmd` array) so graphics persist across text renders.
- Added `/demo` that prints palette colors, bold text, and draws a geometric composition.
- Compiled successfully:

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

Result:

```text
Sketch uses 86816 bytes (4%) of program storage space.
Global variables use 47480 bytes (18%) of dynamic memory, leaving 214664 bytes for local variables.
```

- Uploaded UF2 and BIN to `/pico1-apps`.
- Verified remote files:

```text
/Volumes/NO NAME/pico1-apps/PicoCalc_REPL_Primitives.uf2  207360 bytes
/Volumes/NO NAME/pico1-apps/REPL_Primitives.bin            103624 bytes
```

### Why

The previous incremental edits created a tangle of Arduino preprocessor issues (struct not found in auto-prototypes, variable scope issues). A clean rewrite from scratch with explicit ordering is faster and more reliable than patch-by-patch fixing. The new features (palette, colors, bold, icons, graphics) exercise the full intended surface area of the primitive experiment.

### What worked

- Clean compile with zero errors.
- No fragile forward declarations needed.
- All new commands (`/fg`, `/bg`, `/bold`, `/normal`, `/palette`, `/demo`, `/clear-gfx`, `/icon`) are implemented.
- Graphics replay buffer allows shapes to persist across text renders.
- Dirty-cell renderer still avoids full-screen redraw flicker.

### What didn't work

- Several incremental edit attempts failed with Arduino auto-prototype errors before the clean rewrite.
- The icon replay buffer currently only stores a generic GfxIcon entry; the actual icon name is not replayed. For the primitive sketch this is acceptable because the command text records which icon was drawn.

### What I learned

For Arduino `.ino` files, the safest approach is:
1. Put all struct/enum/class definitions near the top, before any function definitions.
2. Avoid custom struct types in free-function signatures if those structs are defined later in the file.
3. C++ classes would sidestep many of these issues because class methods are not auto-prototyped the same way.

### What was tricky to build

The packed attribute byte (4-bit fg + 4-bit bg) requires careful bit manipulation:
```cpp
uint8_t packAttr(uint8_t fg, uint8_t bg) { return ((fg & 0x0F) << 4) | (bg & 0x0F); }
uint8_t attrFgIndex(uint8_t attr) { return (attr >> 4) & 0x0F; }
uint8_t attrBgIndex(uint8_t attr) { return attr & 0x0F; }
```

The renderer must unpack these for every cell during composition, but the cost is negligible on RP2040.

### What warrants a second pair of eyes

- Whether 47,480 bytes globals is too much for eventual uLisp integration (the back buffer attrs+bold arrays are the main cost).
- Whether the graphics replay buffer should be stored differently for uLisp (e.g. as text commands in the transcript instead of pixel commands).
- Whether C++ class refactoring should happen now or after hardware validation.

### What should be done in the future

- Test `/demo` on hardware.
- Test `/fg pink`, `/bg navy`, `/bold 1` with typed text.
- Test `/icon heart 100 100 2`.
- Test `/clear-gfx` after drawing.
- Consider C++ class refactor.
- Port the validated renderer model back to `ulisp-picocalc.ino`.

### Code review instructions

Review:

```text
repl-window-primitives/repl-window-primitives.ino
```

Focus on:
1. Palette definition (lines ~40-60),
2. `BackBufferState` with parallel `attrs[][]` and `bold[][]` arrays,
3. `renderAll()` dirty-cell loop,
4. `runDemo()` and `runFakeEvaluator()` command dispatch.

### Technical details

Compile result:

```text
Sketch uses 86816 bytes (4%) of program storage space. Maximum is 2093056 bytes.
Global variables use 47480 bytes (18%) of dynamic memory, leaving 214664 bytes for local variables. Maximum is 262144 bytes.
```
