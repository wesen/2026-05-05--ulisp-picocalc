---
Title: Dirty Cell Renderer Design and Implementation Guide
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
      Note: Primitive implementation of lightweight char+attribute dirty-cell renderer
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Future uLisp integration target for the renderer
ExternalSources: []
Summary: Design and implementation guide for replacing full-screen TFT redraws with a lightweight char+attribute dirty-cell renderer suitable for the primitive sketch and later uLisp integration.
LastUpdated: 2026-05-06T03:20:00-04:00
WhatFor: Explain and guide the flicker fix for the PicoCalc REPL window renderer
WhenToUse: When working on primitive rendering or porting the renderer back into ulisp-picocalc.ino
---


# Dirty Cell Renderer Design and Implementation Guide

## Problem

The primitive sketch originally flickered because it rendered directly to the TFT by clearing rows and redrawing text. The PicoCalc display path does not provide an application-level double-buffered framebuffer. Each `fillRect()` and `drawChar()` call sends pixels over SPI directly into the TFT controller's GRAM. If the sketch clears a row and then redraws the same text, the user can see the blank interval between those operations.

The screen may flicker even when the final text is unchanged, because the intermediate state is visible:

```text
old pixels → fillRect black/status background → draw text again
```

The first mitigation was to redraw only when UI state changes, but that still flickers on every changed frame if the renderer clears whole rows. The real fix is to avoid clearing unchanged pixels.

## Constraints

A full pixel framebuffer is too expensive for the eventual uLisp integration.

```text
320 × 320 pixels × 2 bytes RGB565 = 204,800 bytes
```

The RP2040 has 264 KB RAM total, and uLisp needs most of that for its workspace and runtime state. Therefore the renderer should buffer semantic text cells, not pixels.

The PicoCalc text grid is small:

```text
Columns = 320 / 6  = 53
Lines   = 320 / 10 = 32
Cells   = 53 × 32  = 1,696
```

A cell containing one character and one attribute byte costs 2 bytes. Two screens — desired and drawn — cost:

```text
1,696 cells × 2 bytes × 2 buffers = 6,784 bytes
```

This is acceptable in the primitive sketch and a plausible upper bound for uLisp. If uLisp needs to be tighter, use a single drawn-cell buffer plus local composition for dirty regions.

## Design

The renderer has two text-cell buffers:

```cpp
struct RenderCell {
  char ch;
  uint8_t attr;
};

RenderCell desiredCells[Lines][Columns];
RenderCell drawnCells[Lines][Columns];
bool drawnCellsValid;
```

- `desiredCells` is recomposed from application state each render.
- `drawnCells` records what was last sent to the TFT.
- A cell is redrawn only if `ch` or `attr` differs.

Attributes encode colors without storing 16-bit foreground/background colors in every cell:

```cpp
enum CellAttr : uint8_t {
  AttrNormal = 0,
  AttrStatus = 1,
  AttrPrompt = 2,
  AttrCursor = 3,
};
```

Color lookup happens during drawing:

```cpp
uint16_t attrFg(uint8_t attr);
uint16_t attrBg(uint8_t attr);
```

This makes cells compact and keeps the renderer easy to port.

## Rendering Pipeline

```text
BackBuffer + EditBuffer
        ↓
clear desiredCells to blank normal cells
        ↓
compose transcript rows
        ↓
compose status row
        ↓
compose input prompt/text/cursor
        ↓
for each screen cell:
    if desired != drawn:
        redraw that one cell
        drawn = desired
```

Pseudocode:

```cpp
void renderAll() {
  clearDesiredCells();
  composeTranscript();
  composeStatus();
  composeInput();

  for row in Lines:
    for col in Columns:
      if !drawnCellsValid || desired[row][col] != drawn[row][col]:
        drawCellDirect(row, col);
        drawn[row][col] = desired[row][col];

  drawnCellsValid = true;
}
```

## Drawing One Cell

The renderer still clears a small cell rectangle before drawing the character, because a new character may be narrower than the old character or the cell may become blank. The important change is that it clears only a 6×10 cell, not a whole 320-pixel row or the whole screen.

```cpp
void drawCellDirect(int row, int col) {
  const RenderCell &cell = desiredCells[row][col];
  int x = col * CharWidth;
  int y = row * Leading;
  uint16_t bg = attrBg(cell.attr);

  tft.fillRect(x, y, CharWidth, Leading, bg);
  if (cell.ch != ' ') {
    tft.drawChar(x, y, cell.ch, attrFg(cell.attr), bg, 1);
  }
}
```

This is not zero-flicker in the strict hardware sense — a changed cell still goes blank for a moment — but the visible blank area is tiny and only appears where text actually changed.

## Interaction with Dirty UI Flag

A separate `uiDirty` flag prevents rendering when no state has changed:

```cpp
bool uiDirty = true;

void requestRender() { uiDirty = true; }

void loop() {
  pollKeyboard();
  if (uiDirty) {
    renderAll();
    uiDirty = false;
  }
  delay(20);
}
```

The dirty-cell renderer and `uiDirty` solve different problems:

- `uiDirty` avoids unnecessary render passes.
- cell diffing avoids unnecessary TFT pixel writes during a render pass.

Both are needed.

## Primitive Implementation Status

Implemented in:

```text
repl-window-primitives/repl-window-primitives.ino
```

Key symbols:

```cpp
RenderCell
CellAttr
attrFg
attrBg
desiredCells
drawnCells
clearDesiredCells
composeTranscript
composeStatus
composeInput
renderAll
```

Compilation result after implementation:

```text
Sketch uses 78512 bytes (3%) of program storage space.
Global variables use 26672 bytes (10%) of dynamic memory.
```

The RAM increase is intentional and much smaller than the earlier `ScreenCell` foreground/background approach.

## Porting Notes for uLisp

When porting back to `ulisp-picocalc.ino`:

1. Keep the renderer text-cell based, not pixel-framebuffer based.
2. Prefer `char + uint8_t attr` cells.
3. Use attributes for status/cursor/highlight instead of storing colors per cell.
4. Do not call `pserial()` from rendering.
5. `pserial()` should mutate transcript state and request render, not draw directly.
6. `gserial()` / keyboard handlers should mutate the edit buffer and request render.
7. If RAM pressure is too high, eliminate `desiredCells` and compose one row at a time against `drawnCells`.

## Validation Plan

On hardware:

1. Boot the primitive sketch.
2. Confirm there is no idle flicker.
3. Type normal text and confirm only changed input cells update.
4. Press Backspace and confirm only affected input cells update.
5. Run `/spam` and confirm large output is much less flickery than the old full-row renderer.
6. Run `/clear` and confirm the screen updates cleanly.
7. Test arrow/candidate keys and verify status/transcript changes remain stable.

If flicker remains visible, next options are:

- avoid `fillRect()` for non-blank character-to-character updates where `drawChar` with background is enough,
- batch adjacent dirty cells into windows,
- add row-level dirty spans,
- reduce cursor redraw frequency,
- use TFT sprites for only the input line if RAM permits.
