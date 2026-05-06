---
Title: Integrating REPL Window Header into ulisp-picocalc.ino
Ticket: repl-window
Status: active
Topics:
    - picocalc
    - repl
    - display
    - keyboard
    - editor
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repl-window-primitives/repl-window-primitives.ino
      Note: Source primitive sketch used to design header
    - Path: ulisp-picocalc/repl_window.h
      Note: Header extracted from primitive REPL window for uLisp integration
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Integration target for pserial/gserial/setup/repl
ExternalSources: []
Summary: Design for extracting the proven REPL window primitives into a C++ header and wiring them into the uLisp firmware via pserial/gserial/ProcessKey replacements.
LastUpdated: 2026-05-06T03:40:00-04:00
WhatFor: Guide the port from primitive experiment to integrated uLisp REPL window
WhenToUse: When modifying ulisp-picocalc.ino to use the new REPL window
---


# Integrating REPL Window Header into ulisp-picocalc.ino

## Goal

Take the proven primitive components (dirty-cell renderer, back buffer, edit buffer, palette) and integrate them into the real uLisp firmware so that:

- `pserial()` appends output to the back buffer instead of calling `Display()` directly.
- `gserial()` returns committed edit-buffer input instead of raw `KybdBuf` characters.
- `ProcessKey()` edits the input line instead of appending directly to the terminal.
- `repl()` stays conceptually unchanged — it still calls `readmain(gserial)` and `printobject(result, pserial)`.

The primitive sketch's REPL-specific code is extracted into a standalone C++ header so the single `.ino` file stays manageable.

## Files

- **Header:** `ulisp-picocalc/repl_window.h` — extracted from primitive sketch.
- **Integration target:** `ulisp-picocalc/ulisp-picocalc.ino` — modified at specific functions.

## Header contents (`repl_window.h`)

Everything the uLisp `.ino` needs, nothing it doesn't:

- Display geometry constants (`Columns`, `Lines`, `CharWidth`, `Leading`, etc.)
- 16-color palette
- `BackBufferState` with parallel `attrs[][]` and `bold[][]`
- `EditBufferState`
- `DrawState`
- Back-buffer append functions
- Edit-buffer mutation functions
- Dirty-cell renderer functions
- `ReplWindowInit()`
- `ReplProcessKey()`

**Not included:** fake evaluator, graphics commands, icon drawing, Arduino `setup()`/`loop()`.

## Integration points in ulisp-picocalc.ino

### 1. `setup()` — add initialization

After `initgfx()`, call `ReplWindowInit()` to clear buffers and mark `drawnCellsValid = false`.

### 2. `pserial(char c)` — redirect output

Current behavior:
```c
void pserial(char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) Display(c);
  #if defined(serialmonitor)
  Serial.write(c);
  #endif
}
```

New behavior (behind `#ifdef replwindowsupport`):
```c
void pserial(char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) {
    #if defined(replwindowsupport)
    ReplBackBufferAppend(c);
    #else
    Display(c);
    #endif
  }
  #if defined(serialmonitor)
  Serial.write(c);
  #endif
}
```

`ReplBackBufferAppend(c)` does what `appendBackChar()` did in the primitive: append to back buffer with current draw state, handle newline, wrap, and mark `uiDirty`.

### 3. `gserial()` — return committed input

Current behavior: poll keyboard, call `ProcessKey()`, return `KybdBuf` characters.

New behavior (PicoCalc keyboard path, behind `#ifdef replwindowsupport`):
```c
int gserial() {
  while (!EditBuffer.committed) {
    poll keyboard events
    ReplProcessKey(temp);
    if (uiDirty) ReplRenderAll();
  }
  // Return committed characters one by one
  if (EditBuffer.read_pos < EditBuffer.len) return EditBuffer.text[EditBuffer.read_pos++];
  EditBufferResetAfterCommit();
  return '\n';
}
```

Serial monitor path can either:
- bypass the new UI and feed characters directly (existing behavior), or
- be normalized through the same commit path.

For v1, keep serial monitor bypassing the new UI to avoid breaking existing workflows.

### 4. `ProcessKey(char c)` — edit the input line

Current behavior: append to `KybdBuf`, call `Display(c)`, handle backspace via terminal mutations.

New behavior (behind `#ifdef replwindowsupport`):
```c
void ProcessKey(char c) {
  #if defined(replwindowsupport)
  ReplProcessKey(c);
  #else
  // existing terminal-based implementation
  #endif
}
```

`ReplProcessKey(c)`:
- Escape → set `ESCAPE` flag (existing behavior)
- Printable → insert at edit cursor
- Backspace → delete before cursor
- Left/Right → move cursor (if confirmed key codes available)
- Enter → commit input (append prompt+input to back buffer, mark committed)
- Tab → placeholder for autocomplete (restore later)

### 5. `repl()` — add render after output

The `repl()` loop is infinite and never returns. Rendering must happen either:
- inside `gserial()` while polling for input, or
- explicitly after `printobject()` completes.

For v1, add a render call after the print phase:
```c
void repl(object *env) {
  for (;;) {
    ...
    pserial('>'); pserial(' ');
    object *line = readmain(gserial);
    ...
    printobject(line, pserial);
    unprotect();
    pfl(pserial);
    pln(pserial);
    #if defined(replwindowsupport)
    ReplRenderIfDirty();
    #endif
  }
}
```

This ensures output is visible immediately after evaluation, not just when the user starts typing again.

### 6. Old terminal functions — deprecate for REPL text

`Display(c)` and `ScrollDisplay()` should no longer be called for REPL text output. They can be kept for:
- The Lisp editor (`edit()` function) if it still uses terminal emulation
- Graphics functions if they still use terminal control characters
- `VT` (vertical tab) handling for the screen editor

For v1, leave `Display()` in place but don't call it from `pserial()` when `replwindowsupport` is enabled.

## Compile flag

Add `#define replwindowsupport` near the other compile flags (line ~10-21). This makes the new REPL window an opt-in feature.

```c
#define printfreespace
#define serialmonitor
#define sdcardsupport
#define gfxsupport
#define replwindowsupport    // <-- new
```

## Validation checklist

- [ ] uLisp boots with `replwindowsupport` enabled.
- [ ] Banner text appears in the back buffer and renders to TFT.
- [ ] Prompt `> ` appears in the input line.
- [ ] Typing text appears only in the input line.
- [ ] Enter submits the line, appends it to the transcript, evaluates it, and prints the result.
- [ ] Result output appears in the transcript area.
- [ ] Backspace works in the input line.
- [ ] Error recovery works (longjmp back to `loop()`, then fresh REPL restart).
- [ ] `NOECHO` paths still suppress display output.
- [ ] Serial monitor still mirrors output.
- [ ] Compiles with `replwindowsupport` disabled and behavior is unchanged.

## Porting order

1. Create `repl_window.h` from primitive sketch.
2. Add `replwindowsupport` compile flag.
3. Modify `pserial()` to call `ReplBackBufferAppend()`.
4. Modify `gserial()` to use edit buffer.
5. Modify `ProcessKey()` to delegate to `ReplProcessKey()`.
6. Add `ReplWindowInit()` to `setup()`.
7. Add `ReplRenderIfDirty()` to `repl()`.
8. Compile with flag enabled.
9. Compile with flag disabled to confirm no regression.
10. Flash and test on hardware.
