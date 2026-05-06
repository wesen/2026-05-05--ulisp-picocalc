---
Title: Primitive PicoCalc Screen and Keyboard Experiment Sketch
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
    - Path: arduino_picocalc_kbd/src/PCKeyboard.cpp
      Note: Keyboard I2C implementation and key FIFO behavior
    - Path: arduino_picocalc_kbd/src/PCKeyboard.h
      Note: Keyboard API used by the primitive sketch
    - Path: repl-window-primitives/repl-window-primitives.ino
      Note: Standalone C++ Arduino sketch implementing PicoCalc display/keyboard primitives
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Original uLisp firmware whose display/keyboard contracts the primitive sketch de-risks
ExternalSources: []
Summary: Design for a C++-only PicoCalc Arduino sketch that validates TFT rendering, PCKeyboard input, edit-buffer behavior, transcript buffering, and key-code discovery before integrating the REPL window into uLisp.
LastUpdated: 2026-05-06T02:55:00-04:00
WhatFor: Plan the primitive experiment sketch used to de-risk the uLisp REPL window implementation
WhenToUse: Before changing the uLisp interpreter, or when validating PicoCalc screen/keyboard behavior independently
---


# Primitive PicoCalc Screen and Keyboard Experiment Sketch

## Goal

Create a separate Arduino `.ino` sketch that contains only PicoCalc hardware primitives and small C++ UI data structures. This sketch is intentionally independent from uLisp. It exists so we can experiment with the hard parts of the REPL window — display layout, keyboard events, edit-buffer mutation, transcript buffering, cursor drawing, and special-key discovery — before touching the interpreter.

Implementation file:

```text
repl-window-primitives/repl-window-primitives.ino
```

This sketch should answer practical hardware questions:

- Does the TFT initialize correctly using the same `TFT_eSPI` setup as uLisp?
- Does `PCKeyboard` work on `Wire1` with `SDA=6`, `SCL=7`, address `0x1f`?
- What key codes are emitted for arrows, Delete, Escape, Home/End, Tab, and shifted keys?
- Can we redraw the whole screen often enough for a simple v1 renderer?
- How much RAM does a transcript buffer consume?
- How should an editable input line be represented and rendered?

## Why This Exists

The uLisp firmware is a single 7,000+ line `.ino` containing the interpreter, reader, evaluator, printer, display terminal, keyboard processing, filesystem, graphics, and hardware support. Changing the REPL window inside that file directly would mix two risks:

1. UI/hardware risk: screen drawing, key codes, cursor behavior, buffering.
2. Interpreter risk: reader/evaluator/printer contracts, error recovery, `NOECHO`, prompt handling.

The primitive sketch isolates the first risk. Once the primitives are understood, we can port the proven pieces back into `ulisp-picocalc/ulisp-picocalc.ino` with much higher confidence.

## Non-goals

This sketch does **not** implement:

- uLisp objects,
- Lisp parsing,
- evaluation,
- `readmain(gserial)`,
- `printobject(result, pserial)`,
- garbage collection,
- stream dispatch,
- filesystem functions.

It is not a replacement firmware. It is a laboratory.

## Hardware Stack

The experiment uses the same libraries and pins as the current PicoCalc uLisp build.

| Subsystem | Library/API | Configuration |
|---|---|---|
| Display | `TFT_eSPI` | `tft.init()`, `invertDisplay(1)`, 320×320 screen |
| Keyboard | `PCKeyboard` | `Wire1`, `SDA=6`, `SCL=7`, I2C addr `0x1f` |
| Serial debug | Arduino `Serial` | 115200 baud |
| Build target | `arduino-cli` | `--fqbn rp2040:rp2040:rpipico` |

Reference code in uLisp:

- `initgfx()` in `ulisp-picocalc/ulisp-picocalc.ino` initializes the TFT.
- `initkybd()` initializes `Wire1` and `PCKeyboard`.
- `gserial()` polls `pc_kbd.keyCount()` and `pc_kbd.keyEvent()`.

## Target Screen Layout

The primitive sketch uses the same conceptual layout as the future REPL window:

```text
┌────────────────────────────────────┐
│ transcript/back buffer viewport    │
│ previous committed input           │
│ previous output / key logs         │
│ ...                                │
├────────────────────────────────────┤
│ status: rows/view/cursor/len       │
├────────────────────────────────────┤
│ > editable input line              │
│ wrapped input if needed            │
│ live cursor                        │
└────────────────────────────────────┘
```

On the PicoCalc display:

- screen = 320×320 pixels,
- cell width = 6 pixels,
- row leading = 10 pixels,
- columns = `320 / 6 = 53`,
- rows = `320 / 10 = 32`,
- transcript rows = `Lines - StatusRows - InputRows`.

## Data Model

### Back Buffer

The back buffer stores physical display rows, not logical lines. This is deliberate for the experiment: physical rows are easy to render and map directly to the TFT grid.

```cpp
struct BackBufferState {
  char rows[BackBufferRows][Columns];
  uint16_t currentRow;
  uint16_t currentCol;
  uint16_t count;
  uint16_t viewportStart;
  bool followTail;
};
```

Responsibilities:

- append printable characters,
- convert newline into row advance,
- wrap long text at `Columns`,
- keep a ring buffer of recent rows,
- track the visible viewport,
- optionally follow the newest output.

### Edit Buffer

The edit buffer stores the active typed line.

```cpp
struct EditBufferState {
  char text[InputBufferSize];
  uint16_t len;
  uint16_t cursor;
};
```

Responsibilities:

- insert at cursor,
- backspace before cursor,
- delete at cursor,
- move cursor,
- commit current line to the transcript,
- render prompt + text + cursor.

## Control Flow

```text
setup()
  ├── initialize Serial
  ├── clear buffers
  ├── initialize TFT
  ├── initialize PCKeyboard on Wire1
  ├── write startup messages to back buffer
  └── render full UI

loop()
  ├── pollKeyboard()
  │     └── while keyCount > 0: keyEvent → processPrintableOrControl
  ├── renderAll()
  └── delay(20)
```

## Key Handling Strategy

The experiment treats normal printable keys as edit-buffer inserts. It treats Enter as commit. It treats Backspace/Delete as editing actions. For unknown or special keys, it writes a key-code log line into the transcript.

Candidate special codes are based on the current uLisp filter list:

```cpp
0xA1, 0xA2, 0xA3, 0xA4, 0xA5
```

The existing uLisp `gserial()` ignores those values, which strongly suggests they are navigation or special keys. The primitive sketch maps them tentatively and logs them so hardware testing can confirm or correct the mapping.

## Rendering Strategy

Version 1 uses full-screen redraw:

```text
renderAll()
  ├── renderTranscript()
  ├── renderStatus()
  └── renderInput()
```

This is intentionally simple. It makes correctness visible and avoids dirty-region bugs. If full redraw flickers or is too slow on hardware, the next task is to add dirty rows.

Rendering must not call `pserial()` or any print helper from uLisp. In the primitive sketch, rendering writes directly through `tft.drawChar()` and `tft.fillRect()`.

## Success Criteria

The experiment is successful when:

- The sketch compiles as a standalone Arduino sketch.
- The PicoCalc shows startup messages.
- Typing text updates only the bottom edit area.
- Enter appends `> input` and `echo: input` into the transcript and clears the edit area.
- Backspace and Delete mutate the edit buffer.
- Candidate arrow keys either move cursor/scroll or log clear key-code messages.
- The status row updates cursor, length, and viewport information.
- We can record actual key codes for all non-printable keys.

## Port-back Plan to uLisp

Once this primitive sketch is validated on hardware, port in this order:

1. Copy the back-buffer state and append logic into the PicoCalc terminal section of `ulisp-picocalc.ino`.
2. Copy the renderer as `RenderReplWindow()` / `RenderBackBufferViewport()` / `RenderEditBuffer()`.
3. Redirect `pserial()` to append output to the back buffer.
4. Replace `ProcessKey()` internals with edit-buffer mutation.
5. Change `gserial()` so it returns committed edit-buffer characters.
6. Re-implement autocomplete and parenthesis highlighting against `EditBufferState`.

## Build Command

```bash
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```
