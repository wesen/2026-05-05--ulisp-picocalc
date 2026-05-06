# Tasks

## TODO

### Milestone A — Design-preserving scaffolding

- [ ] Add a `replwindowsupport` compile flag near the other compile options in `ulisp-picocalc/ulisp-picocalc.ino`.
- [ ] Add REPL window layout constants derived from existing `Columns` and `Lines`: transcript rows, status rows, and input rows.
- [ ] Add `EditBufferState` with `text[]`, `len`, `cursor`, `committed`, and `read_pos` fields.
- [ ] Add `BackBufferState` with ring-buffer rows, row count/head, current column, viewport top, and follow-tail flag.
- [ ] Add stub functions for `ReplWindowInit`, `ReplBackBufferWrite`, `RenderReplWindow`, `EditBufferReset`, and `ReplProcessKey`.
- [ ] Wire `ReplWindowInit()` into the setup/init path after TFT initialization.
- [ ] Compile with stubs and verify no behavior change when `replwindowsupport` is disabled.

### Milestone B — Back buffer data structure

- [ ] Implement helper to clear a back-buffer row.
- [ ] Implement helper to map logical transcript row indices to physical ring-buffer indices.
- [ ] Implement row advance on newline and wrap.
- [ ] Implement printable character append into the current row.
- [ ] Implement newline handling for `\n` and ignore/normalize `\r`.
- [ ] Implement wrapping when `current_col >= Columns`.
- [ ] Implement tail-following viewport calculation after appended output.
- [ ] Add a temporary debug helper to dump the back buffer to serial for validation.
- [ ] Validate memory impact of the chosen `BackBufferLines` value.

### Milestone C — Renderer / compositor

- [ ] Implement `DrawCell(row, col, ch, fg, bg)` using `tft.drawChar` or safe reuse of `PlotChar`.
- [ ] Implement `RenderBackBufferViewport()` for the transcript area.
- [ ] Implement `RenderStatusRow()` as a separator/status line.
- [ ] Implement `RenderEditBuffer()` to draw the prompt, current input, and cursor.
- [ ] Implement `RenderReplWindow()` as a full-screen redraw for v1.
- [ ] Ensure renderer does not call `pserial()` or any print helper that could recurse back into rendering.
- [ ] Add dirty-row flags only after the full-redraw version is correct.

### Milestone D — Redirect `pserial()` output

- [ ] Modify `pserial(char c)` under `replwindowsupport` to append to `ReplBackBufferWrite(c)` instead of calling `Display(c)`.
- [ ] Preserve `LastPrint = c` behavior in `pserial()`.
- [ ] Preserve serial monitor mirroring in `pserial()`.
- [ ] Preserve `NOECHO` suppression semantics in `pserial()`.
- [ ] Verify boot banner and prompt appear through the back-buffer renderer.
- [ ] Verify result output from `printobject(result, pserial)` appears through the back-buffer renderer.
- [ ] Verify error output appears through the back-buffer renderer.

### Milestone E — Editable input line

- [ ] Implement `EditBufferReset()` without disturbing committed input before `gserial()` consumes it.
- [ ] Implement insertion at cursor with bounds checks.
- [ ] Implement backspace before cursor.
- [ ] Implement delete at cursor.
- [ ] Implement cursor-left and cursor-right movement.
- [ ] Implement Home/End movement if usable key codes are available.
- [ ] Implement `EditBufferCommit()` to append submitted `> input` to the back buffer and mark input committed.
- [ ] Render the cursor at the correct edit-buffer column.
- [ ] Handle long input wrapping or define a v1 limit/error behavior.

### Milestone F — Keyboard processing and `gserial()` integration

- [ ] Refactor `ProcessKey(char c)` so the new path delegates to `ReplProcessKey(c)`.
- [ ] Keep `KEY_ESC` behavior: pressing Escape sets `ESCAPE`.
- [ ] Update `gserial()` so it blocks/polls until `Edit.committed` is true for PicoCalc keyboard input.
- [ ] Update `gserial()` so it returns committed edit-buffer characters one at a time.
- [ ] Update `gserial()` so it returns a final newline exactly once after the committed line.
- [ ] Reset edit-buffer state only after `gserial()` has returned the final newline.
- [ ] Remove or isolate dependence on `KybdAvailable`, `ReadPtr`, and `WritePtr` for the new PicoCalc keyboard path.
- [ ] Decide whether serial-monitor input bypasses the UI or is normalized into the same committed-input path.

### Milestone G — Restore existing UX features against new buffers

- [ ] Reimplement Tab autocomplete using `Edit.text` and `Edit.cursor` instead of `KybdBuf` and terminal backtracking.
- [ ] Reimplement parenthesis matching as render-time highlighting over `Edit.text`.
- [ ] Preserve or replace `SHIFTRETURN` last-input recall.
- [ ] Add a real input-history ring separate from the transcript back buffer.
- [ ] Add Up/Down history recall when focus is in the input line.
- [ ] Add scrollback key bindings, preferably Shift+Up/Shift+Down or another non-conflicting chord.
- [ ] Add status row indicators for scroll position and follow-tail state.
- [ ] Ensure break-level prompts still render correctly.

### Milestone H — Robustness and validation

- [ ] Test `(+ 1 2)` evaluates to `3` with the new input path.
- [ ] Test quoted lists, strings, `defun`, and subsequent function calls through `readmain(gserial)`.
- [ ] Test long output and confirm wrapping/scrolling does not corrupt the edit line.
- [ ] Test long input near the edit-buffer limit.
- [ ] Test back-buffer rollover after more than `BackBufferLines` rows.
- [ ] Test error recovery with invalid expressions and divide-by-zero.
- [ ] Test Escape abort behavior.
- [ ] Test `NOECHO` paths, paste/listing behavior, and library loading if applicable.
- [ ] Compile and flash on hardware, then record observed key codes for arrows/Home/End/Delete.
- [ ] Update `docs/ulisp-picocalc-source-index.md` after implementation line numbers change.

### Documentation and handoff

- [ ] Keep `reference/01-intern-guide-building-the-repl-window-in-ulisp-picocalc-ino.md` updated as implementation decisions change.
- [ ] Update `design/01-repl-window-layout-design.md` when key bindings and layout are finalized.
- [ ] Record any failed approaches and hardware observations in the ticket changelog or diary.
- [ ] Add screenshots/photos of the finished REPL window to the ticket.

## Primitive C++ PicoCalc Experiment Tasks

### Primitive Milestone P0 — Standalone sketch foundation

- [x] Create `repl-window-primitives/repl-window-primitives.ino` as a separate Arduino sketch.
- [x] Initialize TFT display with `TFT_eSPI` independently from uLisp.
- [x] Initialize PicoCalc keyboard with `PCKeyboard` on `Wire1` SDA=6/SCL=7 address `0x1f`.
- [x] Compile the primitive sketch with `arduino-cli`.
- [ ] Flash the primitive sketch to PicoCalc hardware and confirm boot display.

### Primitive Milestone P1 — Back buffer and renderer

- [x] Implement physical-row transcript/back buffer.
- [x] Implement append, newline, wrap, and tail-follow behavior.
- [x] Implement full-screen renderer with transcript, status row, and input area.
- [x] Test full-screen redraw on hardware for flicker/performance; user reported screen flicker while other behavior seemed OK.
- [x] Add lightweight dirty-cell rendering to avoid full row/screen clears after flicker report.

### Primitive Milestone P2 — Keyboard and edit buffer

- [x] Implement mutable input buffer with insert/backspace/delete/cursor movement.
- [x] Implement Enter commit from edit buffer to transcript.
- [x] Log unknown non-printable key events.
- [ ] Record actual key codes for arrows, Delete, Home, End, Tab, Escape, and shifted combinations.
- [ ] Correct tentative `0xA1`–`0xA5` mappings based on hardware results.

### Primitive Milestone P3 — UX experiments before uLisp port

- [ ] Add input history ring to primitive sketch.
- [ ] Add scrollback mode using confirmed key bindings.
- [x] Add fake evaluator output mode for deterministic multi-line output stress tests.
- [ ] Add parenthesis highlighting against `EditBufferState`.
- [ ] Add autocomplete prototype against `EditBufferState`.

### Primitive Milestone P4 — Port-back readiness

- [ ] Document final primitive APIs that should port to `ulisp-picocalc.ino`.
- [ ] Compare primitive sketch behavior against existing uLisp `Display()`, `ProcessKey()`, `gserial()`, and `pserial()` contracts.
- [ ] Decide which primitive code can be copied directly and which must be adapted for uLisp flags such as `NOECHO`, `ESCAPE`, and break-level prompts.
- [ ] Update the intern guide with hardware findings from the primitive sketch.

### Primitive Milestone P5 — Colors, bold, icons, and graphics

- [x] Add 16-color neon/neotokyo palette.
- [x] Add per-cell fg/bg color attributes via packed byte in back buffer.
- [x] Add bold rendering via double-draw offset.
- [x] Add simple icon drawing (heart, star, check, cross, arrows).
- [x] Add TFT graphics commands: line, rect, fill-rect, circle, fill-circle.
- [x] Add `/demo` command that exercises colors, bold, and graphics.
- [x] Add `/fg`, `/bg`, `/bold`, `/normal`, `/palette`, `/clear-gfx` commands.
- [x] Add `/icon name x y col` command.
- [ ] Test `/demo` on hardware and verify all 16 colors, bold text, and graphics overlay.
- [ ] Test `/clear-gfx` removes graphics overlay without corrupting text.


### Integration Milestone I1 — Header extraction and uLisp wiring

- [x] Create `ulisp-picocalc/repl_window.h` from primitive REPL window components.
- [x] Remove fake evaluator, icons, and graphics commands from the uLisp integration header.
- [x] Include `repl_window.h` from `ulisp-picocalc/ulisp-picocalc.ino`.
- [x] Replace `pserial()` display path with `ReplBackBufferAppend()`.
- [x] Replace `gserial()` PicoCalc keyboard path with committed edit-buffer input.
- [x] Initialize the REPL window in `setup()` after TFT init.
- [x] Render the REPL window after printed evaluation output.
- [x] Compile integrated uLisp sketch successfully.
- [ ] Reduce integrated RAM usage below the current 227,760-byte globals footprint.
- [ ] Hardware-test boot, input editing, Enter commit, evaluation output, and error recovery.

