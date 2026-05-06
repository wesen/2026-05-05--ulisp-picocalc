---
Title: Color Control Codes, Bold, and TFT Graphics Primitives
Ticket: repl-window
Status: active
Topics:
    - picocalc
    - repl
    - display
    - graphics
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/repl-window-primitives/repl-window-primitives.ino
ExternalSources: []
Summary: "Add a 16-entry neon/neotokyo palette, per-cell foreground and background color support via control codes, a bold rendering attribute, an icon/emoji set, and direct TFT drawing commands for lines, rectangles, and circles."
LastUpdated: 2026-05-06T03:35:00-04:00
WhatFor: "Extend the primitive experiment with rich visual output and graphics primitives before uLisp integration."
WhenToUse: "When adding color, styling, or drawing capabilities to the REPL window renderer."
---

# Color Control Codes, Bold, and TFT Graphics Primitives

## Goal

Extend the primitive sketch with:

1. a 16-color neon / neotokyo palette,
2. per-cell foreground and background color stored in a compact attribute byte,
3. control codes that set the current draw state which subsequent text inherits,
4. a bold text style rendered via repeated character draws,
5. an icon/emoji mode that substitutes short names with pictograms,
6. TFT draw commands: line, rect, fill-rect, circle, fill-circle.

## Palette

16 RGB565 entries.  Bright neons on dark backgrounds.

```cpp
const uint16_t Palette[16] = {
  0x0000,  //  0  void black
  0xCE79,  //  1  soft white
  0xF81F,  //  2  neon pink    R=31 G=0  B=16
  0x07E0,  //  3  neon green   R=0  G=63 B=0
  0x07FF,  //  4  neon cyan    R=0  G=63 B=31
  0xFFE0,  //  5  neon yellow  R=31 G=63 B=0
  0xD81F,  //  6  neon purple  R=27 G=0  B=16
  0xFD20,  //  7  neon orange  R=31 G=20 B=0
  0x001F,  //  8  electric blue R=0  G=0  B=31
  0xF8B9,  //  9  hot magenta  R=31 G=5  B=9
  0xA7E4,  // 10  acid lime    R=20 G=62 B=4
  0x87FF,  // 11  ice blue     R=16 G=63 B=31
  0xFC80,  // 12  coral        R=31 G=18 B=0
  0x05BF,  // 13  deep cyan    R=0  G=45 B=31
  0xBA9A,  // 14  muted pink   R=23 G=10 B=13
  0x10A2,  // 15  dark navy    R=2  G=5  B=2
};
```

Default: fg = 1 (soft white), bg = 0 (void black).

## Compact Attribute Byte

Back-buffer rows carry a parallel `attrs[Columns]` byte array.  One byte
per cell, encoding:

```text
bits 7-4   foreground color index (0-15)
bits 3-0   background color index (0-15)
```

Bold is a separate rendering flag on DrawState, not a per-cell attribute.
The renderer draws bold characters by repeating the `drawChar` call one
pixel to the right.

Icons are mapped by a small static table of short-ascii names to custom
glyphs drawn by the renderer.

## Control Codes

A stateful `DrawState` is maintained on the append side:

```cpp
struct DrawState {
  uint8_t fg;
  uint8_t bg;
  bool bold;
};
```

When the fake evaluator encounters these commands, the state is updated.
Subsequent transcript output inherits the current state until a new
command changes it.

Commands:

```text
/fg N     foreground color   (N decimal 0-15 or name pink/green/cyan/…)
/bg N     background color
/bold     toggle bold
/bold 1   bold on
/bold 0   bold off
/normal   reset to default fg/bg/bold
/icon X   draw icon by name (arrow-left, heart, star, etc.)
/palette  print palette swatch rows
```

Color names are mapped by a small table:

```text
black  0   white  1   pink   2   green  3   cyan   4
yellow 5   purple 6   orange 7   blue   8   magenta 9
lime   10  ice    11  coral  12  deep   13  muted  14  navy 15
```

## Bold Rendering

When `drawCellDirect` is called for a cell that is bold:

```cpp
tft.drawChar(x,     y, cell.ch, fg, bg, 1);  // normal
tft.drawChar(x + 1, y, cell.ch, fg, bg, 1);  // bold offset
```

The cell width is 6 px, so adding a 1 px shift produces a bold appearance
on this small font.

## Icon Table

Small set of named icons, stored as 6×8 pixel bitmaps in program memory
(PROGMEM).  Drawn with `tft.drawBitmap` or pixel loops.

Initial icons:

```text
arrow-left  arrow-right  arrow-up  arrow-down
heart  star  check  cross  bullet  block
```

Fake-evaluator command:

```text
/icon heart
```

The icon is drawn once into the transcript (not a persistent cell
attribute).  For the primitive sketch it is enough that the icon name is
echoed in the transcript and the renderer draws the bitmap at that cell
position.

## TFT Drawing Commands

Direct pixel drawing on the TFT.  These bypass the text-cell render.
The transcript receives a log line but the pixel region is not part of
the text buffer back-store.

Commands:

```text
/line      x0 y0 x1 y1 col
/rect      x  y  w  h  col
/fillrect  x  y  w  h  col
/circle    x  y  r     col
/fillcircle x y  r     col
/clear-gfx                erase any previous pixel graphics
```

Coordinates are in pixels.  Colors are palette indices (0-15).
`/clear-gfx` fills the whole TFT with black and retriggers a full text
render.

## Fake Evaluator Command Summary

After this change the fake evaluator understands:

```text
/help /spam /clear /status              ← existing
/fg /bg /bold /normal /icon /palette    ← new color/style
/line /rect /fillrect /circle /fillcircle /clear-gfx  ← new graphics
```

## Extensions to the Back Buffer

The back-buffer row struct gains a parallel attribute array:

```cpp
uint8_t backAttrs[BackBufferRows][Columns];
```

`appendBackChar` writes the current DrawState's packed attribute byte
alongside the character.

Memory cost:

```text
160 rows × 53 bytes = 8,480 bytes
```

Total back buffer: 8,480 (chars) + 8,480 (attrs) = 16,960 bytes.

The primitive sketch has ~235 KB free RAM, so this is acceptable.

When porting to uLisp, reduce `BackBufferRows` if needed or make the
attribute array optional.

## Renderer Changes

`composeTranscript` now reads both the character and the attribute byte.
It uses the attribute to resolve foreground and background colors:

```cpp
uint8_t fgIdx = attr >> 4;
uint8_t bgIdx = attr & 0x0F;
uint16_t fg = Palette[fgIdx];
uint16_t bg = Palette[bgIdx];
```

Set the desired cell with appropriate colors from the attribute byte.

`AttrStatus`, `AttrPrompt`, and `AttrCursor` override the per-cell
attribute for the status bar and input line.

## UI Flow

```text
loop()
  pollKeyboard()
  if uiDirty:
    graphics overlay? → draw graphics primitives
    renderAll()        → dirty-cell text render
    uiDirty = false
```

Graphics are drawn after text composition but before the render loop ends
so they appear on top of text.  This is v1 semantics; the design doc for
the final uLisp port should define whether text and graphics layers are
separate or overlaid.

## Validation Plan

On hardware:

1. Type `/fg 2` then "hello" → pink hello appears in transcript.
2. Type `/bg 15` then "/fg 3 hi" → green on navy text.
3. Type `/bold 1` then "BOLD" → bold text appears.
4. Type `/normal` → default white on black resumes.
5. Type `/palette` → sixteen color swatch rows render.
6. Type `/line 10 10 100 100 2` → pink diagonal line.
7. Type `/fillrect 50 50 60 30 4` → cyan filled rect.
8. Type `/circle 160 130 40 6` → purple circle outline.
9. Type `/fillcircle 200 200 30 7` → orange filled circle.
10. Type `/clear-gfx` → graphics removed, text returns.

## `/demo` Command

The primitive sketch includes a convenience command:

```text
/demo
```

It exercises the major primitives together:

- prints palette/color samples,
- prints bold and normal text,
- draws a navy graphics background,
- draws filled and outlined circles,
- draws diagonal lines,
- draws filled and outlined rectangles,
- draws a small icon.

This should be the first hardware smoke test after flashing the build.

## C++ Organization Note

Arduino `.ino` preprocessing can create fragile auto-prototypes for free functions that use custom types. For the primitive sketch, the current cleaned implementation avoids custom types in problematic free-function signatures and keeps struct definitions before use.

For the next cleanup, it would be better to move the major subsystems into C++ classes, for example:

```cpp
class BackBuffer;
class EditBuffer;
class DirtyRenderer;
class GraphicsLayer;
class CommandShell;
```

Class methods avoid many of the Arduino auto-prototype pitfalls and make the eventual uLisp port easier to reason about.

## Implementation File

All changes in:

```text
repl-window-primitives/repl-window-primitives.ino
```