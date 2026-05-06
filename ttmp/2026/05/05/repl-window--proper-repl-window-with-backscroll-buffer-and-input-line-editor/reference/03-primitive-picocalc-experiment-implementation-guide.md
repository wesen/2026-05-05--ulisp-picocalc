---
Title: Primitive PicoCalc Experiment Implementation Guide
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
      Note: Implementation target for primitive experiments
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/02-primitive-picocalc-screen-and-keyboard-experiment-sketch.md
      Note: Design document for primitive experiment
ExternalSources: []
Summary: Implementation guide for the standalone C++ PicoCalc primitive experiment sketch.
LastUpdated: 2026-05-06T02:56:00-04:00
WhatFor: Guide future work on the primitive display/keyboard sketch
WhenToUse: When adding features to repl-window-primitives before porting them to uLisp
---


# Primitive PicoCalc Experiment Implementation Guide

## Goal

This guide explains how to work on the standalone sketch:

```text
repl-window-primitives/repl-window-primitives.ino
```

The sketch is a hardware/UI proving ground for the future uLisp REPL window. It validates display and keyboard primitives in isolation.

## Quick Reference

Build:

```bash
cd /home/manuel/code/wesen/2026-05-05--ulisp-picocalc
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --build-path build-repl-window-primitives \
  --warnings all \
  repl-window-primitives
```

Primary file:

```text
/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino
```

Fake evaluator commands:

```text
/help    show command help
/spam    print 48 deterministic long lines for wrap/scroll testing
/clear   clear the transcript back buffer
/status  print buffer and input status
```

Expected compile warnings:

- `TOUCH_CS pin not defined` from `TFT_eSPI`; this is expected because the sketch does not use touch.

## Current Components

### Display constants

The sketch uses fixed character-cell geometry:

```cpp
constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 320;
constexpr int CharWidth = 6;
constexpr int Leading = 10;
constexpr int Columns = ScreenWidth / CharWidth;
constexpr int Lines = ScreenHeight / Leading;
```

These match the existing uLisp PicoCalc terminal assumptions.

### Back buffer APIs

Important functions:

```cpp
void appendBackChar(char c);
void appendBackString(const char *s);
void appendBackLine(const char *s);
void advanceBackRow();
void scrollBackBufferToTail();
```

Use these functions for transcript output. Do not draw transcript text directly from key handlers.

### Edit buffer APIs

Important functions:

```cpp
void resetEditBuffer();
bool insertEditChar(char c);
bool backspaceEditChar();
bool deleteEditChar();
void moveCursorLeft();
void moveCursorRight();
void commitEditBuffer();
```

Use these functions for active input state. Do not mutate `editBuffer.text` directly outside these helpers.

### Renderer APIs

Important functions:

```cpp
void renderTranscript();
void renderStatus();
void renderInput();
void renderAll();
```

The renderer is currently a full redraw. Optimize only after correctness is verified on hardware.

### Keyboard APIs

Important functions:

```cpp
void pollKeyboard();
void processPrintableOrControl(uint8_t key);
void logKeyEvent(uint8_t key, PCKeyboard::KeyState state);
```

`pollKeyboard()` drains all available key events each loop. Unknown non-printable key events are logged into the transcript so hardware testing can discover real key codes.

## Implementation Rules

- Keep the sketch uLisp-free.
- Keep UI state explicit: back buffer for transcript, edit buffer for current input.
- Commit small changes and compile after each one.
- Record hardware findings in the diary.
- When a primitive becomes stable, document how it should port back to `ulisp-picocalc.ino`.

## Next Development Steps

1. Flash and test on hardware.
2. Record actual key codes for arrows/Delete/Home/End/Tab/Escape.
3. Fix tentative key-code mappings.
4. Add input history ring.
5. Add scrollback mode independent from key-code logs.
6. Add dirty-row renderer if full redraw flickers.
7. Add a fake evaluator mode that prints multi-line deterministic output for stress testing.

## Validation Script for Humans

On hardware:

1. Boot the sketch.
2. Confirm the startup text appears.
3. Type `hello`.
4. Confirm `hello` appears only in the bottom edit area.
5. Type `/help` and press Enter to verify command output.
6. Type `/spam` and press Enter to verify multi-line wrapping and scroll stress output.
7. Press Backspace.
8. Confirm the edit area changes to `hell`.
9. Press Enter.
10. Confirm transcript receives:

```text
> hell
echo: hell
```

11. Press arrows/Delete/Home/End and write down logged key codes.
12. Generate enough committed lines to fill the screen and confirm scrolling.
