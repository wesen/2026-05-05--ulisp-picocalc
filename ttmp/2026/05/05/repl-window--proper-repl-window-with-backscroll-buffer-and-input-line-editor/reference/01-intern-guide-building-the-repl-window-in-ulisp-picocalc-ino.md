---
Title: 'Intern Guide: Building the REPL Window in ulisp-picocalc.ino'
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
    - Path: docs/ulisp-picocalc-source-index.md
      Note: Line-index reference used to orient intern to source sections 25-28
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/01-repl-window-layout-design.md
      Note: Companion design interpretation document
    - Path: ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/repl-window-layout.png
      Note: Original sketch for input edit buffer
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Implementation target; contains pserial
ExternalSources: []
Summary: A new-intern technical guide for implementing the PicoCalc REPL window remodel inside ulisp-picocalc.ino, including current architecture, target architecture, APIs, pseudocode, file references, tasks, and validation steps.
LastUpdated: 2026-05-06T02:35:00-04:00
WhatFor: Onboarding and implementation guidance for the REPL backscroll/input-line project
WhenToUse: Before and during implementation of the REPL window in ulisp-picocalc/ulisp-picocalc.ino
---


# Intern Guide: Building the REPL Window in `ulisp-picocalc.ino`

## Goal

Build a proper PicoCalc REPL window for uLisp by replacing the current direct terminal-style screen mutation with a small UI subsystem built from:

- an **editable input line / edit buffer** for the expression currently being typed,
- a **committed-input stream** consumed by the existing Lisp reader,
- a **back buffer / transcript buffer** containing submitted input and evaluator output,
- a **viewport renderer** that draws the transcript plus active input area onto the PicoCalc TFT display,
- a redirected **`pserial()` output path** that appends printed output to the transcript instead of directly drawing terminal characters.

The implementation target is:

```text
/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/ulisp-picocalc.ino
```

The source index reference is:

```text
/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/docs/ulisp-picocalc-source-index.md
```

The hand-drawn design sketch is stored in this ticket:

```text
ttmp/2026/05/05/repl-window--proper-repl-window-with-backscroll-buffer-and-input-line-editor/design/repl-window-layout.png
```

## The Mental Model

The current firmware behaves like an old terminal: each key and each printed character is immediately applied to the display using `Display(c)`. That is simple, but it means the current input and historical output are mixed together as one stream of terminal mutations.

The target design separates those concerns.

The user should type into a **mutable editor**. The editor can move the cursor, insert text, delete text, and redraw the active line without polluting output history. Only when the user presses Enter does that input become a committed Lisp expression. The submitted line is then appended to the transcript/back buffer and exposed to the existing Lisp reader as a normal character stream.

Evaluator output should not directly draw into the edit buffer. Instead, all output produced by `printobject(..., pserial)`, `pfstring(..., pserial)`, errors, prompts, and helper printing functions should go through a single transcript append API. The display renderer then paints the visible portion of that transcript and the active edit buffer.

In one sentence:

> Keep `repl()` conceptually unchanged, but change what `gserial()` and `pserial()` mean.

## Correct Reading of the Design Sketch

The sketch shows this flow:

```text
key
  ↓
input line
  ↓
display in edit buffer, with cursor
  ↓ Enter
input is submitted and edit buffer is cleared
  ↓
eval
  ├── add input to buffer
  └── add output to buffer
          ↑
          red note: redirect pserial to do that
  ↓
back buffer display
```

Important details from the sketch:

- `key → input line`: raw keyboard events edit the active input line.
- `display in edit buffer`: current typed text is displayed from edit-buffer state.
- `cursor`: cursor belongs to the editable input line, not to the transcript.
- Enter submits the current input and clears the edit buffer.
- Submitted input is added to the buffer.
- Evaluator output is added to the buffer.
- The red note says to redirect `pserial` to make output buffering happen.
- The bottom-right screen labeled `back buffer` is the rendered REPL history/transcript.

## Current Architecture in `ulisp-picocalc.ino`

This project is a single-file firmware. There are no separate UI source files yet. The display, keyboard, reader, printer, and REPL are all in `ulisp-picocalc/ulisp-picocalc.ino`.

The sections that matter are near the bottom of the file.

### Printer / output functions

File reference:

```text
ulisp-picocalc/ulisp-picocalc.ino:6976+
```

Current `pserial()`:

```c
void pserial (char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) Display(c);         // Don't display when paste in listing
  #if defined (serialmonitor)
  if (c == '\n') Serial.write('\r');
  Serial.write(c);
  #endif
}
```

Current behavior:

- Records the last printed char in `LastPrint`.
- If `NOECHO` is not set, calls `Display(c)` directly.
- If `serialmonitor` is enabled, mirrors output to serial.

Target behavior:

- Keep `LastPrint = c`.
- Keep optional serial monitor output.
- Replace direct display mutation with transcript append:

```c
if (!tstflag(NOECHO)) ReplBackBufferWrite(c);
```

### Terminal display functions

File reference:

```text
ulisp-picocalc/ulisp-picocalc.ino:7221+
```

Important constants:

```c
const int ScreenWidth = 320, ScreenHeight = 320;
const int CharWidth = 6, CharHeight = 8, Leading = 10;
const int Columns = ScreenWidth/CharWidth;
const int TextWidth = Columns*CharWidth;
const int Lines = ScreenHeight/Leading;
const int LastColumn = Columns-1;
const int LastLine = Lines-1;
const char Cursor = 0x5f;
```

Derived values on the PicoCalc:

- `Columns = 320 / 6 = 53`
- `Lines = 320 / 10 = 32`
- `LastColumn = 52`
- `LastLine = 31`

Current display primitives:

```c
void PlotChar(uint8_t ch, uint8_t line, uint8_t column);
void ScrollDisplay();
void Display(char c);
```

Current behavior:

- `Display(c)` is a mini terminal emulator.
- It keeps static `line` and `column` cursor state.
- Printable chars are plotted immediately.
- Newline advances cursor or calls `ScrollDisplay()`.
- Backspace and cursor movement are simulated with control characters.
- `ScrollBuf[Columns][Lines]` holds the current visible screen state, not a long transcript.

Target behavior:

- `PlotChar()` can remain the low-level draw primitive.
- `Display()` should no longer be the primary output model for REPL text.
- `ScrollDisplay()` should be replaced or bypassed for REPL transcript scrolling.
- Rendering should become explicit:

```text
clear/redraw transcript viewport rows
clear/redraw separator/status row
clear/redraw edit-buffer rows and cursor
```

### Keyboard and input buffer

File reference:

```text
ulisp-picocalc/ulisp-picocalc.ino:7319+
```

Current keyboard globals:

```c
volatile int WritePtr = 0, ReadPtr = 0, LastWritePtr = 0;
const int KybdBufSize = Columns*Lines;
char KybdBuf[KybdBufSize], ScrollBuf[Columns][Lines];
volatile uint8_t KybdAvailable = 0;
uint8_t Scroll = 0;
```

Current `ProcessKey(c)` responsibilities:

- Escape sets the `ESCAPE` flag.
- Maintains parenthesis highlighting.
- Newline commits the keyboard buffer:

```c
pserial('\n');
KybdAvailable = 1;
ReadPtr = 0;
LastWritePtr = WritePtr;
return;
```

- Backspace mutates `KybdBuf` only at the end.
- `SHIFTRETURN` restores last input.
- Printable characters append to `KybdBuf` and immediately call `Display(c)`.

Current limitations:

- Editing is append/backspace only.
- The live input buffer and screen output are coupled.
- Cursor movement inside the input line is not supported.
- The same display path handles live typing and evaluator output.

Target behavior:

- `ProcessKey()` should become a dispatcher into `EditBufferHandleKey()`.
- The active input should support cursor-based editing.
- Committed input should be distinct from raw keyboard input.
- Parenthesis highlighting should operate on `EditBuffer.text`, not `KybdBuf` + terminal backtracking.

### Reader and `gserial()`

File reference:

```text
ulisp-picocalc/ulisp-picocalc.ino:7464+
```

Current `gserial()` behavior:

- Polls serial monitor and PicoCalc keyboard until `KybdAvailable` is true.
- Calls `ProcessKey(temp)` for each key press.
- Once Enter has made the keyboard buffer available, returns chars from `KybdBuf` one by one.
- After all chars are consumed, resets the buffer and returns newline.

Current shape:

```c
while (!KybdAvailable) {
  poll serial / keyboard;
  ProcessKey(temp);
}
if (ReadPtr != WritePtr) return KybdBuf[ReadPtr++];
KybdAvailable = 0;
WritePtr = 0;
return '\n';
```

Target behavior:

- Keep `readmain(gserial)` unchanged.
- Make `gserial()` read from a committed input queue produced by the edit buffer.
- While no committed input is available, pump key events and render the UI.

### REPL loop

File reference:

```text
ulisp-picocalc/ulisp-picocalc.ino:7724+
```

Current `repl()`:

```c
void repl (object *env) {
  for (;;) {
    randomSeed(micros());
    #if defined(printfreespace)
    if (!tstflag(NOECHO)) gc(NULL, env);
    pint(Freespace+1, pserial);
    #endif
    if (BreakLevel) {
      pfstring(" : ", pserial);
      pint(BreakLevel, pserial);
    }
    pserial('>'); pserial(' ');
    Context = NIL;
    object *line = readmain(gserial);
    ...
    protect(line);
    pfl(pserial);
    line = eval(line, env);
    pfl(pserial);
    printobject(line, pserial);
    unprotect();
    pfl(pserial);
    pln(pserial);
  }
}
```

Target approach:

- Prefer keeping this loop mostly unchanged.
- It should still print prompts through `pserial()`.
- It should still call `readmain(gserial)`.
- It should still call `printobject(line, pserial)`.
- The difference is that `pserial()` writes to the back buffer and `gserial()` returns committed edit-buffer input.

This minimizes interpreter risk.

## Target Architecture

### High-level diagram

```text
┌─────────────────────┐
│ PicoCalc keyboard   │
│ pc_kbd.keyEvent()   │
└──────────┬──────────┘
           │ raw key press
           ▼
┌─────────────────────┐
│ EditBuffer          │
│ - text[]            │
│ - len               │
│ - cursor            │
│ - committed flag    │
└──────────┬──────────┘
           │ Enter
           ▼
┌─────────────────────┐
│ CommittedInput      │
│ chars consumed by   │
│ gserial()           │
└──────────┬──────────┘
           │ gserial chars
           ▼
┌─────────────────────┐
│ readmain(gserial)   │
│ existing Lisp reader│
└──────────┬──────────┘
           │ Lisp object
           ▼
┌─────────────────────┐
│ eval()              │
└──────────┬──────────┘
           │ result / printed output
           ▼
┌─────────────────────┐
│ pserial(c)          │
│ redirected output   │
└──────────┬──────────┘
           │ append char
           ▼
┌─────────────────────┐
│ BackBuffer          │
│ transcript ring     │
└──────────┬──────────┘
           │ render viewport
           ▼
┌─────────────────────┐
│ TFT display         │
│ transcript + input  │
└─────────────────────┘
```

### Component responsibilities

#### `EditBuffer`

A mutable buffer for the expression currently being typed.

API responsibilities:

- Initialize/reset state.
- Insert a character at cursor.
- Delete before cursor.
- Delete at cursor.
- Move cursor left/right/home/end.
- Commit current text on Enter.
- Render itself with cursor.

Suggested shape:

```c
const int EditBufferSize = KybdBufSize; // v1: reuse existing capacity

struct EditBufferState {
  char text[EditBufferSize];
  uint16_t len;
  uint16_t cursor;
  bool committed;
  uint16_t read_pos;
};

EditBufferState Edit;
```

#### `CommittedInput`

For v1, the edit buffer itself can be the committed input source. When Enter is pressed:

- `Edit.committed = true`
- `Edit.read_pos = 0`
- `gserial()` returns `Edit.text[Edit.read_pos++]`
- when `read_pos == len`, it returns `\n`
- then it resets the edit buffer

Later, this can become a queue if multi-line paste or asynchronous submission is needed.

#### `BackBuffer`

An append-only transcript buffer for input and output.

V1 can store physical screen rows rather than logical lines. That is simpler because the renderer already uses fixed-width character cells.

Suggested constants:

```c
const int BackBufferLines = 160;          // tune after memory check
const int BackBufferCols = Columns;       // 53 on PicoCalc
const int ViewRowsReservedForInput = 2;   // prompt/input area v1
const int StatusRows = 1;
const int TranscriptRows = Lines - ViewRowsReservedForInput - StatusRows;
```

Memory estimate:

```text
160 rows × 53 chars = 8,480 bytes
```

That is reasonable on RP2040 if no excessive attribute arrays are added. For v1, avoid per-character color attributes unless required.

Suggested shape:

```c
struct BackBufferState {
  char rows[BackBufferLines][BackBufferCols];
  uint16_t head;          // next row to write
  uint16_t count;         // number of valid rows, max BackBufferLines
  uint16_t current_row;   // logical current append row
  uint16_t current_col;
  uint16_t viewport_top;  // logical row index shown at top
  bool follow_tail;       // true means auto-scroll to newest output
};

BackBufferState Back;
```

Important design choice:

- `head` and `count` manage circular storage.
- `current_col` manages wrapping and newline handling.
- `viewport_top` manages scrollback display.
- `follow_tail` determines whether new output auto-scrolls the viewport.

#### Renderer / Compositor

The renderer draws the current UI state:

```text
rows 0..N          transcript viewport
row N+1            separator/status row
bottom row(s)      prompt + edit buffer + cursor
```

A first implementation can redraw the whole screen after each key or output char. That is easiest to validate. If it flickers or is slow, optimize later with dirty rows.

Recommended v1 approach:

- Implement `RenderReplWindow()` full redraw.
- Implement `RenderBackBufferViewport()` for transcript rows.
- Implement `RenderStatusRow()`.
- Implement `RenderEditBuffer()`.
- Later add dirty flags.

## API Reference to Add

Add these functions near the current PicoCalc terminal/keyboard support section, after constants and before keyboard processing.

### Initialization

```c
void ReplWindowInit();
```

Purpose:

- Clear `Back` and `Edit` state.
- Clear TFT display.
- Set `Back.follow_tail = true`.
- Render initial empty UI.

Call from:

```c
setup() or initgfx()/after initgfx()
```

Recommended call site:

```c
initgfx();
ReplWindowInit();
initkybd();
```

If `ReplWindowInit()` uses only TFT and constants, it can run before `initkybd()`.

### Back buffer append

```c
void ReplBackBufferWrite(char c);
void ReplBackBufferWriteString(const char *s);
void ReplBackBufferNewline();
```

Purpose:

- Append printable characters to the current transcript row.
- On newline, advance to a new row.
- On wrap, advance to a new row.
- Maintain `count`, `head`, `current_col`, and `viewport_top`.

Pseudocode:

```c
void ReplBackBufferWrite(char c) {
  if (c == '\r') return;

  if (c == '\n') {
    ReplBackBufferNewline();
    ReplRenderMaybe();
    return;
  }

  if ((c & 0x7f) < 32) {
    handle_control_char_or_ignore(c);
    ReplRenderMaybe();
    return;
  }

  Back.rows[Back.head][Back.current_col] = c;
  Back.current_col++;

  if (Back.current_col >= BackBufferCols) {
    ReplBackBufferNewline();
  }

  if (Back.follow_tail) ReplScrollToTail();
  ReplRenderMaybe();
}
```

### Rendering

```c
void RenderReplWindow();
void RenderBackBufferViewport();
void RenderStatusRow();
void RenderEditBuffer();
void DrawCell(uint8_t row, uint8_t col, char c, uint16_t fg, uint16_t bg);
```

Purpose:

- Draw transcript rows from `Back`.
- Draw status/separator row.
- Draw active input line and cursor from `Edit`.

Initial full-redraw pseudocode:

```c
void RenderReplWindow() {
  #if defined(gfxsupport)
  tft.fillScreen(TFT_BLACK);
  RenderBackBufferViewport();
  RenderStatusRow();
  RenderEditBuffer();
  #endif
}
```

A later optimized renderer can replace full-screen clearing with dirty-row updates.

### Edit buffer operations

```c
void EditBufferReset();
bool EditBufferInsert(char c);
bool EditBufferBackspace();
bool EditBufferDelete();
void EditBufferMoveLeft();
void EditBufferMoveRight();
void EditBufferMoveHome();
void EditBufferMoveEnd();
void EditBufferCommit();
```

Pseudocode for insertion:

```c
bool EditBufferInsert(char c) {
  if (Edit.len >= EditBufferSize - 1) return false;

  for (int i = Edit.len; i > Edit.cursor; i--) {
    Edit.text[i] = Edit.text[i - 1];
  }
  Edit.text[Edit.cursor] = c;
  Edit.cursor++;
  Edit.len++;
  Edit.text[Edit.len] = 0;
  return true;
}
```

Pseudocode for backspace:

```c
bool EditBufferBackspace() {
  if (Edit.cursor == 0) return false;

  for (int i = Edit.cursor - 1; i < Edit.len - 1; i++) {
    Edit.text[i] = Edit.text[i + 1];
  }
  Edit.cursor--;
  Edit.len--;
  Edit.text[Edit.len] = 0;
  return true;
}
```

Pseudocode for commit:

```c
void EditBufferCommit() {
  // Add the submitted line to visible transcript.
  ReplBackBufferWrite('>');
  ReplBackBufferWrite(' ');
  for (int i = 0; i < Edit.len; i++) ReplBackBufferWrite(Edit.text[i]);
  ReplBackBufferWrite('\n');

  // Make it available to readmain(gserial).
  Edit.committed = true;
  Edit.read_pos = 0;

  // Do not clear text immediately if gserial() must consume from it.
  // Clear after consumption completes.
}
```

### Key handling

```c
void ReplProcessKey(char c);
void PumpKeyboardUntilInputCommitted();
```

Responsibilities:

- Translate raw key codes into edit operations.
- Preserve Escape handling.
- Decide how to handle Tab autocomplete.
- Decide how to handle Shift+Return or history recall.

Pseudocode:

```c
void ReplProcessKey(char c) {
  if (c == KEY_ESC) {
    setflag(ESCAPE);
    return;
  }

  switch (c) {
    case '\n':
    case '\r':
      EditBufferCommit();
      break;
    case 8:
    case 0x7f:
      EditBufferBackspace();
      break;
    default:
      if ((c & 0x7f) >= 32) EditBufferInsert(c);
      break;
  }

  RenderReplWindow();
}
```

### `gserial()` contract after remodel

`gserial()` must still satisfy the Lisp reader API:

```c
int gserial();
```

Contract:

- Return one input character at a time.
- Block/poll until input exists.
- Return newline after the committed line.
- Reset committed state after the line is consumed.
- Continue supporting serial monitor if required.

Pseudocode:

```c
int gserial() {
  #if defined(serialmonitor)
  // Optional: serial input path can remain, but be careful not to bypass UI.
  #endif

  while (!Edit.committed) {
    PumpKeyboardOnce();
  }

  if (Edit.read_pos < Edit.len) {
    return Edit.text[Edit.read_pos++];
  }

  EditBufferReset();
  return '\n';
}
```

## Implementation Plan

### Phase 0: Add compile-safe scaffolding

Goal: introduce types and functions without changing behavior yet.

Steps:

- Add `EditBufferState` and `BackBufferState` structs.
- Add constants for back buffer size and layout row counts.
- Add empty/stub functions:
  - `ReplWindowInit()`
  - `ReplBackBufferWrite()`
  - `RenderReplWindow()`
  - `EditBufferReset()`
  - `ReplProcessKey()`
- Compile after every small change.

Validation:

- Firmware compiles with no behavior change.

### Phase 1: Implement back buffer only

Goal: make `pserial()` append to an in-memory transcript while still optionally using old `Display(c)` behind a flag during transition.

Suggested transition implementation:

```c
#define REPL_WINDOW_EXPERIMENT 1

void pserial(char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) {
    #if REPL_WINDOW_EXPERIMENT
    ReplBackBufferWrite(c);
    #else
    Display(c);
    #endif
  }
  ... serial monitor mirror ...
}
```

Validation:

- Back buffer receives banner text and prompt.
- Serial monitor output still works.
- No crash on boot.

### Phase 2: Render transcript viewport

Goal: draw the back buffer to the TFT.

Start simple:

- Full-screen redraw in `RenderReplWindow()`.
- Use `PlotChar()` or direct `tft.drawChar()`.
- Reserve bottom row for edit buffer.
- Reserve one row for status/separator.

Validation:

- Boot banner appears.
- Prompt appears.
- Evaluation output appears in transcript area.
- Long output wraps.
- Newlines advance rows.

### Phase 3: Implement edit buffer rendering

Goal: display active input separately from transcript.

Steps:

- Add `RenderEditBuffer()`.
- Draw `> ` then `Edit.text`.
- Invert or draw `_` at `Edit.cursor`.
- Clear the input area before redrawing it.

Validation:

- Typing appears in the bottom input area.
- Backspace updates input area.
- Cursor appears in correct position.

### Phase 4: Refactor key handling

Goal: raw keys mutate `EditBuffer`, not `KybdBuf` + terminal display.

Steps:

- Replace `ProcessKey()` internals with `ReplProcessKey()` or have `ProcessKey()` delegate.
- Keep `KEY_ESC` behavior.
- Implement printable insert and backspace.
- Enter calls `EditBufferCommit()`.
- Temporarily disable parenthesis highlighting if necessary; restore later against `EditBuffer.text`.

Validation:

- Typing no longer appends to transcript until Enter.
- Pressing Enter adds submitted input to transcript.
- After Enter, `readmain(gserial)` receives the expression.

### Phase 5: Refactor `gserial()`

Goal: `gserial()` returns committed edit-buffer characters.

Steps:

- Replace `KybdAvailable`, `ReadPtr`, `WritePtr` usage with `Edit.committed` and `Edit.read_pos` for PicoCalc keyboard path.
- Keep serial monitor path if needed.
- Ensure newline is returned exactly once after a committed line.
- Reset edit buffer after consumption.

Validation:

- `(+ 1 2)` evaluates to `3`.
- Empty input does not corrupt reader state.
- Multiple evaluations work in a row.
- Errors recover and the next input line works.

### Phase 6: Scrollback controls

Goal: allow the viewport to move through history.

Steps:

- Define key bindings, e.g. Shift+Up/Shift+Down for transcript scroll.
- Add `Back.follow_tail = false` when user scrolls up.
- Add `Back.follow_tail = true` when user scrolls to bottom or submits input.
- Render scroll position in status row.

Validation:

- Generate >32 rows of output.
- Scroll up and down.
- New output auto-scrolls only when following tail.

### Phase 7: Restore advanced features

Goal: bring back features that may be paused during the refactor.

Features:

- Tab autocomplete.
- Parenthesis highlighting.
- Input history recall.
- Break-level prompt display.
- `NOECHO` paste/listing semantics.
- Beep/control characters.
- Existing editor `VT` behavior, if still needed.

## Tricky Parts and Pitfalls

### Do not feed raw keys directly to the reader

The reader expects a stream of expression characters. If raw arrow keys, UI-control keys, or partial edited state leak into `readmain(gserial)`, parsing will break.

Only committed input should be exposed to `readmain()`.

### Do not clear the edit buffer before `gserial()` consumes it

On Enter, mark the buffer committed, but keep the text intact until `gserial()` has returned every character. Clear only after returning the final newline.

Bad:

```c
EditBufferCommit();
EditBufferReset(); // loses text before reader gets it
```

Good:

```c
EditBufferCommit(); // committed=true, read_pos=0
// gserial consumes text later
// reset after final newline
```

### Avoid recursive output/render loops

If rendering calls any function that prints through `pserial()`, you may create recursion:

```text
pserial → ReplBackBufferWrite → Render → pserial → ...
```

Rendering functions should draw directly with `tft.drawChar()` or `PlotChar()` if `PlotChar()` does not call `pserial()`.

### Be careful with `NOECHO`

Existing code uses `NOECHO` to suppress display when pasting/listing. `pserial()` currently obeys:

```c
if (!tstflag(NOECHO)) Display(c);
```

The equivalent should be:

```c
if (!tstflag(NOECHO)) ReplBackBufferWrite(c);
```

Do not remove this behavior without understanding paste/listing paths.

### Parenthesis highlighting currently uses terminal backtracking

Existing `Highlight()` works by sending backspaces and special parenthesis control characters to `Display()`:

```c
Display(8);
Display(17 + invert);
Display(9);
```

That model conflicts with a state-rendered edit buffer. Reimplement highlighting as a rendering concern:

- scan `Edit.text` for matching parenthesis,
- remember indices to highlight,
- render those cells inverted in `RenderEditBuffer()`.

### Physical rows vs logical lines

A transcript can be stored as:

- logical lines: one submitted/output line may be longer than screen width,
- physical rows: text is wrapped as it is appended.

For v1, use physical rows. It is simpler and matches the display.

Later, logical lines can support better copying, history, and wrapping behavior.

## Suggested Code Layout in the `.ino`

Place new structs and functions in the PicoCalc terminal/keyboard support area:

```text
// PicoCalc terminal and keyboard support
constants
new REPL window structs/constants
new BackBuffer functions
new EditBuffer functions
new Renderer functions
old/compat PlotChar
keyboard processing
read functions
```

Recommended insertion point:

- after the screen constants and before `PlotChar()`, or
- after `PlotChar()` if you want to reuse it immediately.

Avoid scattering the new subsystem throughout the file. This is a single-file firmware, so locality matters.

## Concrete First Patch Shape

A minimal first patch could look like this:

```c
#define replwindowsupport

const int BackBufferLines = 160;
const int InputRows = 2;
const int StatusRows = 1;
const int TranscriptRows = Lines - InputRows - StatusRows;

struct EditBufferState {
  char text[KybdBufSize];
  uint16_t len;
  uint16_t cursor;
  bool committed;
  uint16_t read_pos;
};

struct BackBufferState {
  char rows[BackBufferLines][Columns];
  uint16_t head;
  uint16_t count;
  uint16_t current_col;
  uint16_t viewport_top;
  bool follow_tail;
};

EditBufferState Edit;
BackBufferState Back;
```

Then adapt `pserial()`:

```c
void pserial (char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) {
    #if defined(replwindowsupport)
    ReplBackBufferWrite(c);
    #else
    Display(c);
    #endif
  }
  #if defined (serialmonitor)
  if (c == '\n') Serial.write('\r');
  Serial.write(c);
  #endif
}
```

Then adapt `ProcessKey()` gradually:

```c
void ProcessKey(char c) {
  #if defined(replwindowsupport)
  ReplProcessKey(c);
  #else
  // existing implementation
  #endif
}
```

During development, keep the old code behind `#else` until the new path is stable.

## Validation Checklist

### Basic boot

- PicoCalc boots.
- Display initializes.
- `uLisp 4.8f` banner appears in transcript.
- Prompt appears.

### Basic input

- Typing `(+ 1 2)` appears only in the edit line.
- Left/right cursor movement works if implemented.
- Backspace works.
- Enter moves `> (+ 1 2)` into transcript.
- Result `3` appears in transcript.
- Edit line clears.

### Reader correctness

Test these inputs:

```lisp
(+ 1 2)
'(1 2 3)
(defun square (x) (* x x))
(square 9)
"hello"
```

Expected:

- reader parses lists, quotes, strings, and symbols correctly,
- output appears after evaluation,
- no stale chars from old input leak into next read.

### Long output and wrapping

Test:

```lisp
(pprintall)
```

or any function that prints many lines.

Expected:

- transcript wraps at screen width,
- old output remains scrollable if scrollback is implemented,
- display does not corrupt the edit line.

### Error recovery

Test:

```lisp
(/ 1 0)
```

Expected:

- error message appears in transcript,
- REPL recovers,
- next input works.

### `NOECHO`

Exercise code paths that set `NOECHO`, especially paste/listing/autoload scenarios if available.

Expected:

- suppressed output remains suppressed from display/back buffer,
- serial behavior remains compatible.

## Detailed Task Breakdown

Use this as the implementation checklist. The ticket `tasks.md` should mirror these tasks.

### Milestone A: Design-preserving scaffolding

- Add `replwindowsupport` compile flag.
- Add layout constants: transcript rows, status rows, input rows.
- Add `EditBufferState` struct.
- Add `BackBufferState` struct.
- Add initialization functions and call them from setup path.
- Compile without behavior change.

### Milestone B: Back buffer

- Implement row clearing.
- Implement row advance.
- Implement character append.
- Implement newline handling.
- Implement wrapping.
- Implement tail-following viewport calculation.
- Add optional debug helper to dump back buffer to serial.

### Milestone C: Rendering

- Implement cell drawing.
- Implement transcript viewport rendering.
- Implement status/separator row.
- Implement edit buffer row rendering.
- Implement full-screen `RenderReplWindow()`.
- Add dirty flags after correctness is proven.

### Milestone D: Output redirection

- Change `pserial()` to call `ReplBackBufferWrite(c)` under feature flag.
- Preserve `LastPrint`.
- Preserve serial monitor mirroring.
- Preserve `NOECHO` behavior.
- Verify prompt/result/error output.

### Milestone E: Edit buffer

- Implement reset.
- Implement insert.
- Implement backspace.
- Implement delete.
- Implement cursor left/right.
- Implement home/end if key codes are available.
- Implement commit.
- Render cursor correctly.

### Milestone F: `gserial()` integration

- Make `gserial()` block/poll until `Edit.committed`.
- Return committed characters one by one.
- Return final newline.
- Reset edit buffer after final newline.
- Remove dependence on `KybdAvailable` for PicoCalc keyboard path.
- Preserve serial monitor behavior or route serial monitor input through the same commit path.

### Milestone G: Restore UX features

- Reimplement autocomplete using `Edit.text` and `Edit.cursor`.
- Reimplement parenthesis matching as render-time highlighting.
- Add input history ring.
- Add history recall key bindings.
- Add scrollback key bindings.
- Add status indicators.

### Milestone H: Robustness

- Test long input near buffer limit.
- Test long output near back buffer rollover.
- Test Escape abort.
- Test break-level prompts.
- Test errors and recovery.
- Test save/load image messages.
- Confirm memory impact.

## Intern Work Rules

- Make one small patch at a time.
- Compile after each patch.
- Do not rewrite evaluator, reader, or printer internals unless necessary.
- Keep `repl()` stable if possible.
- Put all new REPL UI state in one contiguous source region.
- Keep old behavior behind `#else` until new path is stable.
- Prefer simple full redraw first; optimize later.
- Document key-code discoveries in the ticket diary or changelog.

## Quick Reference: Functions to Read Before Coding

| Function | File line area | Why it matters |
|---|---:|---|
| `pserial(char c)` | `ulisp-picocalc.ino:6976` | Output redirect point. |
| `PlotChar(...)` | `ulisp-picocalc.ino:7221` | Low-level character drawing primitive. |
| `ScrollDisplay()` | `ulisp-picocalc.ino:7234` | Current scroll model to replace. |
| `Display(char c)` | `ulisp-picocalc.ino:7263` | Current terminal emulator; understand before bypassing. |
| `initkybd()` | `ulisp-picocalc.ino:7319` | Keyboard setup. |
| `autoComplete()` | `ulisp-picocalc.ino:7329` | Existing autocomplete tied to `KybdBuf`. |
| `Highlight(...)` | `ulisp-picocalc.ino:7383` | Current parenthesis highlight tied to terminal mutation. |
| `ProcessKey(char c)` | `ulisp-picocalc.ino:7393` | Main key dispatch to refactor. |
| `gserial()` | `ulisp-picocalc.ino:7464` | Reader input source to adapt. |
| `readmain(gfun)` | `ulisp-picocalc.ino:7673` | Existing reader entry point; do not break contract. |
| `setup()` | `ulisp-picocalc.ino:7706` | Initialization call site. |
| `repl(object *env)` | `ulisp-picocalc.ino:7724` | Main loop; ideally stable. |
| `ulisperror()` | `ulisp-picocalc.ino:7779` | Error recovery; may need UI reset after errors. |

## Final Target Behavior

When the intern is done, the REPL should feel like this:

1. The top/main area shows scrollable previous REPL transcript.
2. The bottom input area shows the current prompt and editable expression.
3. Typing edits the bottom input line only.
4. Enter submits the line, adds it to transcript, clears the edit area, evaluates it, and appends output to transcript.
5. `pserial()` no longer directly writes terminal characters to the display; it appends output to the back buffer.
6. `gserial()` no longer leaks raw key events to the reader; it supplies committed input chars.
7. The existing uLisp reader/evaluator/printer loop remains mostly intact.
