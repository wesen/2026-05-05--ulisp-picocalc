# Changelog

## 2026-05-05

- Initial workspace created


## 2026-05-05

Created ticket with design doc and layout sketch. Identified 3 screen zones: backscroll buffer, separator/status bar, input line editor. Mapped affected source sections (§25-28). Broke implementation into 3 phases with 9 tasks.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/docs/ulisp-picocalc-source-index.md — Sections 25-28 identify the Display/Keyboard/Reader/REPL functions that need remodeling
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/01-repl-window-layout-design.md — Full design doc with data flow
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/repl-window-layout.png — Hand-drawn layout sketch defining the 3-zone REPL window


## 2026-05-05

Corrected REPL window design interpretation after re-reading the sketch: key input enters an editable input line displayed in an edit buffer; Enter clears/submits input to eval; both submitted input and pserial output are appended to a back buffer; pserial should be redirected to perform output buffering.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/01-repl-window-layout-design.md — Updated design doc with corrected image interpretation and concrete data flow
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/repl-window-layout.png — Source sketch showing edit-buffer input


## 2026-05-05

Added detailed intern-facing implementation guide for building the REPL window in ulisp-picocalc.ino, expanded tasks into milestone-level implementation checklist, and uploaded a bundled PDF to reMarkable at /ai/2026/05/05/repl-window/repl-window-intern-guide.pdf.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/01-intern-guide-building-the-repl-window-in-ulisp-picocalc-ino.md — Detailed analysis/design/implementation guide for a new intern
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/tasks.md — Expanded milestone-level implementation checklist


## 2026-05-05

Created standalone C++ primitive PicoCalc sketch for TFT/keyboard experiments, added separate design and implementation guide, expanded tasks with primitive milestones, compiled successfully, and uploaded the primitive design PDF to reMarkable at /ai/2026/05/05/repl-window/primitive-picocalc-screen-keyboard-design.pdf.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Standalone primitive display/keyboard experiment sketch
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md — Primitive experiment design uploaded to reMarkable
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Diary Step 1 recording initial primitive sketch work
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/03-primitive-picocalc-experiment-implementation-guide.md — Implementation guide for future primitive work


## 2026-05-05

Recorded initial primitive sketch implementation commit c46ef29c3d3f41772148d1aa670fb78e208f270c in the implementation diary.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Diary updated with initial implementation commit hash


## 2026-05-05

Primitive sketch Step 2: added fake evaluator commands (/help, /spam, /clear, /status) for deterministic output, wrapping, and scroll stress tests; compile succeeds.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Added fake evaluator commands and clear-back-buffer helper
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Recorded Step 2
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/03-primitive-picocalc-experiment-implementation-guide.md — Documented fake evaluator commands


## 2026-05-05

Recorded primitive Step 2 implementation commit f9ae1f2028ff8dda3b8f1036dbd1573ada5f55c2 in the diary.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Diary updated with Step 2 commit hash


## 2026-05-05

Built primitive sketch and uploaded artifacts to PicoCalc SD card firmware directory as PicoCalc_REPL_Primitives.uf2 and REPL_Primitives.bin; verified remote files exist.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Source sketch used for uploaded primitive firmware artifacts
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Recorded SD-card upload step


## 2026-05-05

Corrected primitive firmware upload destination to SD-card /pico1-apps; rebuilt and verified PicoCalc_REPL_Primitives.uf2 and REPL_Primitives.bin at /Volumes/NO NAME/pico1-apps.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Source sketch used for corrected /pico1-apps upload
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Recorded corrected SD-card upload destination


## 2026-05-05

Implemented lightweight char+attribute dirty-cell renderer after hardware flicker report; added renderer design/implementation guide; compile succeeds with globals reduced to 26,672 bytes compared with the heavier color-per-cell model.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Replaced row-clearing renderer with dirty text-cell diff renderer
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/03-dirty-cell-renderer-design-and-implementation-guide.md — Design and implementation guide for dirty-cell rendering
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Recorded dirty-cell renderer step


## 2026-05-05

Step 6: Clean rewrite of primitive sketch adding 16-color palette, per-cell fg/bg attributes, bold rendering, icons, TFT graphics commands with replay buffer, /demo command. Compiles cleanly at 86816B flash / 47480B RAM. Uploaded to /pico1-apps.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino — Clean rewrite with palette
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/04-color-control-codes-bold-and-tft-graphics-primitives.md — Design doc for color/graphics primitives
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/02-primitive-experiment-implementation-diary.md — Recorded Step 6


## 2026-05-05

Integration Step 1: extracted primitive REPL window into ulisp-picocalc/repl_window.h, wired pserial/gserial/setup/repl in ulisp-picocalc.ino, renamed comments .ino to .ino.bak so Arduino compiles one sketch, and achieved successful compile; RAM is high at 227760 bytes globals.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/04-integration-implementation-diary.md — Recorded integration Step 1
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/repl_window.h — New REPL window header extracted from primitive sketch
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/ulisp-picocalc.ino — Integrated pserial/gserial/setup/repl with new REPL window


## 2026-05-05

Integration Step 2: reduced RP2040 uLisp WORKSPACESIZE from 23000-SDSIZE to 18000-SDSIZE, improving globals from 227760B to 187760B; compiled and uploaded integrated REPL-window firmware to /pico1-apps.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/04-integration-implementation-diary.md — Recorded heap reduction and SD upload
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/repl_window.h — Integrated REPL-window header
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/ulisp-picocalc.ino — Reduced RP2040 WORKSPACESIZE and integrated REPL-window pserial/gserial/setup/repl changes


## 2026-05-05

Committed integrated REPL-window firmware changes in nested ulisp-picocalc repo as 84a1e97d8d1ddce56399289f9de51d7a9f4a7cf9.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/reference/04-integration-implementation-diary.md — Updated with nested commit hash
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc — Nested firmware repo commit with REPL-window header integration

