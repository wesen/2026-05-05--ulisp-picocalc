# uLisp PicoCalc Experiments

This repository is a working laboratory for learning **uLisp on the Clockwork Pi PicoCalc** and gradually turning the stock console into a richer, window-manager-like environment.  The immediate focus is a new REPL: a proper backbuffer-backed transcript, an editable input line, dirty-cell rendering on the TFT display, and a path toward more structured screen/UI primitives.

The project started as a source/build exploration of Technoblogy's `ulisp-picocalc` firmware and grew into three connected tracks:

1. **Understand uLisp on PicoCalc.** Build it, read the single-file interpreter, index the important source sections, and learn the reader/evaluator/printer/display pipeline.
2. **Prototype UI primitives outside uLisp.** Use a standalone C++ Arduino sketch to experiment with the PicoCalc TFT and keyboard before touching the interpreter.
3. **Integrate a new REPL window into uLisp.** Extract the proven primitive renderer into `repl_window.h`, wire it into `pserial()` and `gserial()`, and make the real Lisp REPL use an editable input line plus transcript backbuffer.

The long-term direction is a small window-manager-like system for uLisp on the PicoCalc: the REPL is the first window, and the graphics/text primitives are the basis for richer interactive tools.

## Current status

The current integrated uLisp build includes:

- a `repl_window.h` header inside the nested `ulisp-picocalc` firmware repo,
- a transcript backbuffer rendered with dirty text-cell updates,
- an editable input line with cursor handling,
- prompt/input/output coloring in the transcript,
- `pserial()` routed through the backbuffer instead of direct `Display(c)` terminal output,
- `gserial()` routed through committed edit-buffer input,
- a reduced uLisp heap on RP2040 to leave RAM for the new UI buffers.

The latest integrated firmware artifacts were uploaded to the PicoCalc SD card as:

```text
/pico1-apps/PicoCalc_uLisp_REPL_Window.uf2
/pico1-apps/uLisp_REPL_Window.bin
```

The standalone primitive experiment is still useful and has its own uploaded build:

```text
/pico1-apps/PicoCalc_REPL_Primitives.uf2
/pico1-apps/REPL_Primitives.bin
```

## Repository layout

```text
.
├── Makefile                         # original build/upload helper for ulisp-picocalc-sketch
├── docs/
│   └── ulisp-picocalc-source-index.md
├── repl-window-primitives/
│   └── repl-window-primitives.ino    # standalone C++ TFT/keyboard/renderer experiment
├── scripts/
│   └── uf2-to-bin.py                 # UF2 → BIN extraction helper
├── ttmp/                             # docmgr ticket workspaces and diaries
├── ulisp-picocalc/                   # nested firmware repo / Arduino sketch
│   ├── repl_window.h                 # integrated REPL window header
│   ├── ulisp-picocalc.ino            # real uLisp PicoCalc firmware
│   └── ulisp-picocalc-comments.ino.bak
└── ulisp-wasm/                       # uLisp WASM exploration sources
```

A detail worth knowing: `ulisp-picocalc/` is itself a Git repository.  Commits to the actual firmware happen there first, and the outer repo records the nested repo pointer plus all ticket docs and primitive experiments.

## Build commands

### Build the primitive UI experiment

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

This builds the C++-only sketch used to test display and keyboard primitives without uLisp.

### Build the integrated uLisp REPL-window firmware

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-ulisp-integrate \
  --warnings all \
  ulisp-picocalc
```

Important Arduino project-structure rule: Arduino compiles **all `.ino` files in the sketch directory**.  The commented source copy is therefore named:

```text
ulisp-picocalc/ulisp-picocalc-comments.ino.bak
```

Do not rename it back to `.ino` inside the sketch folder unless you want duplicate-definition build failures.

## Uploading to the PicoCalc SD card

The active SD-card app directory is:

```text
/Volumes/NO NAME/pico1-apps
```

Example integrated uLisp upload:

```bash
scp build-ulisp-integrate/ulisp-picocalc.ino.uf2 \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/PicoCalc_uLisp_REPL_Window.uf2"

scp build-ulisp-integrate/ulisp-picocalc.ino.bin \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/uLisp_REPL_Window.bin"
```

Example primitive upload:

```bash
scp build-repl-window-primitives/repl-window-primitives.ino.uf2 \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/PicoCalc_REPL_Primitives.uf2"

scp build-repl-window-primitives/repl-window-primitives.ino.bin \
  "manuel@192.168.0.57:/Volumes/NO NAME/pico1-apps/REPL_Primitives.bin"
```

## Main technical ideas

### Backbuffer instead of terminal stream

The original uLisp display path treated the TFT like a terminal.  `pserial(c)` called `Display(c)`, and `Display(c)` directly mutated screen pixels and cursor state.  That works for printing bytes, but it makes editing and scrollback awkward.

The new path treats the REPL as state:

```text
pserial(c) → ReplBackBufferAppend(c) → dirty-cell renderer → TFT
```

Output becomes transcript data before it becomes pixels.

### Editable input line

The keyboard no longer feeds raw characters directly to the reader.  Keypresses mutate an edit buffer.  Only Enter commits the buffer, after which `gserial()` returns the committed characters one by one to `readmain(gserial)`.

```text
keyboard event → edit buffer → Enter → committed input stream → uLisp reader
```

This keeps cursor movement, backspace, delete, and future history recall out of the Lisp reader.

### Dirty-cell rendering

A full pixel framebuffer would cost:

```text
320 × 320 × 2 bytes = 204,800 bytes
```

That is too much RAM for an RP2040 firmware that also hosts uLisp.  The renderer instead stores text cells:

```text
53 columns × 32 rows = 1,696 cells
```

It composes a desired screen, compares it to the last drawn screen, and redraws only changed cells.  This fixed the visible flicker observed in the primitive sketch.

### RAM tradeoff

The integrated REPL window consumes extra globals for the backbuffer and dirty-cell renderer.  To get a practical first firmware, the RP2040 uLisp heap was reduced from:

```cpp
#define WORKSPACESIZE (23000-SDSIZE)
```

to:

```cpp
#define WORKSPACESIZE (18000-SDSIZE)
```

That frees about 40 KB because uLisp objects are 8 bytes each.

## Interesting documents index

The richest project knowledge lives in the docmgr ticket workspaces under `ttmp/2026/05/05/`.  These are the most useful documents to read.

### uLisp source and build exploration

- [`docs/ulisp-picocalc-source-index.md`](docs/ulisp-picocalc-source-index.md) — source map for the 7,000+ line `ulisp-picocalc.ino`; start here if you want to navigate the interpreter.
- [`ttmp/.../ulisp-picocalc/design/01-ulisp-picocalc-build-notes.md`](ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/design/01-ulisp-picocalc-build-notes.md) — build notes and early Arduino setup findings.
- [`ttmp/.../ulisp-picocalc/reference/02-building-ulisp-picocalc-guide.md`](ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/reference/02-building-ulisp-picocalc-guide.md) — practical guide for compiling the firmware.
- [`ttmp/.../ulisp-picocalc/reference/03-questions-and-deep-answers.md`](ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/reference/03-questions-and-deep-answers.md) — deeper Q&A about uLisp internals and PicoCalc behavior.
- [`ttmp/.../ulisp-picocalc/reference/05-understanding-with-output-to-string-in-ulisp-picocalc.md`](ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/reference/05-understanding-with-output-to-string-in-ulisp-picocalc.md) — focused note on one uLisp feature and its implementation implications.

### Console/window-manager architecture

- [`ttmp/.../picocalc-wm/design/01-console-architecture.md`](ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/01-console-architecture.md) — architectural analysis of the original console path.
- [`ttmp/.../picocalc-wm/design/02-literate-console.md`](ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/02-literate-console.md) — literate explanation of the console in a textbook style.
- [`ttmp/.../picocalc-wm/design/03-picocalc-bootloader-analysis.md`](ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/03-picocalc-bootloader-analysis.md) — analysis of PicoCalc bootloader behavior and app-loading constraints.
- [`ttmp/.../picocalc-wm/reference/05-picocalc-uf2loader-two-stage-deep-dive.md`](ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/reference/05-picocalc-uf2loader-two-stage-deep-dive.md) — deep dive into UF2 loader behavior and two-stage loading.

### REPL window design and implementation

- [`ttmp/.../repl-window/design/01-repl-window-layout-design.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/01-repl-window-layout-design.md) — interpretation of the original hand-drawn REPL window sketch.
- [`ttmp/.../repl-window/reference/01-intern-guide-building-the-repl-window-in-ulisp-picocalc-ino.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/01-intern-guide-building-the-repl-window-in-ulisp-picocalc-ino.md) — long-form intern guide to the uLisp integration points.
- [`ttmp/.../repl-window/design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md) — design for the standalone primitive experiment.
- [`ttmp/.../repl-window/reference/03-primitive-picocalc-experiment-implementation-guide.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/03-primitive-picocalc-experiment-implementation-guide.md) — implementation guide for the primitive sketch.
- [`ttmp/.../repl-window/design/03-dirty-cell-renderer-design-and-implementation-guide.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/03-dirty-cell-renderer-design-and-implementation-guide.md) — why full redraw flickers and how dirty-cell rendering fixes it.
- [`ttmp/.../repl-window/design/04-color-control-codes-bold-and-tft-graphics-primitives.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/04-color-control-codes-bold-and-tft-graphics-primitives.md) — palette, bold text, icon, and graphics primitive design from the standalone sketch.
- [`ttmp/.../repl-window/design/05-integrating-repl-window-header-into-ulisp-picocalc-ino.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/05-integrating-repl-window-header-into-ulisp-picocalc-ino.md) — design for extracting `repl_window.h` and wiring it into uLisp.
- [`ttmp/.../repl-window/reference/02-primitive-experiment-implementation-diary.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md) — chronological diary for primitive sketch development.
- [`ttmp/.../repl-window/reference/04-integration-implementation-diary.md`](ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/04-integration-implementation-diary.md) — chronological diary for real uLisp integration.

### WASM side quest

- [`ttmp/.../ulisp-wasm-playground/design/01-playground-guide.md`](ttmp/2026/05/05/ulisp-wasm-playground--build-run-and-improve-the-ulisp-wasm-playground/design/01-playground-guide.md) — notes on building/running a uLisp WASM playground.

## Obsidian deep dive

A durable article-style project report was also written in the Obsidian vault:

```text
/home/manuel/code/wesen/obsidian-vault/Projects/2026/05/05/ARTICLE - PicoCalc uLisp REPL Window - Backbuffer Rendering and RAM-Conscious UI.md
```

It is the best single narrative overview of the backbuffer/REPL integration work.

## Git notes

This repository contains a nested Git repository at `ulisp-picocalc/`.  Push order matters:

1. Push the nested firmware repo first:

   ```bash
   cd ulisp-picocalc
   git push origin main
   ```

2. Then push the outer project repo:

   ```bash
   cd ..
   git push origin main
   ```

If the nested repo remote (`technoblogy/ulisp-picocalc`) is not writable, push that nested repo to a fork/branch first and then decide how the outer repository should reference it.

## Next steps

- Hardware-test the latest `PicoCalc_uLisp_REPL_Window` build.
- Verify prompt/input/output colors and spacing on the physical display.
- Revisit error-color styling for pink error output.
- Reimplement autocomplete and parenthesis matching against `ReplEditBuffer`.
- Decide how aggressively to shrink or optimize the backbuffer once real RAM behavior is known.
- Continue toward a minimal window manager: REPL first, then graphics panes, inspectors, editors, and help windows.
