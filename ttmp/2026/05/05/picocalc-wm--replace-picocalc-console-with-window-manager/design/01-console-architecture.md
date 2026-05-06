---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: PicoCalc/Code/pico_multi_booter/sd_boot/config.h
      Note: Complete pin assignment map (SPI0/1
    - Path: PicoCalc/Code/pico_multi_booter/sd_boot/i2ckbd/i2ckbd.c
      Note: Low-level RP2040 I2C keyboard client
    - Path: PicoCalc/Code/pico_multi_booter/sd_boot/lcdspi/lcdspi.h
      Note: Low-level LCD SPI primitives and color definitions
    - Path: PicoCalc/Code/picocalc_keyboard/picocalc_keyboard.ino
      Note: STM32 keyboard firmware (I2C slave
    - Path: arduino_picocalc_kbd/examples/InterruptDriven/InterruptDriven.ino
      Note: Example of GPIO interrupt-driven keyboard reading
    - Path: arduino_picocalc_kbd/src/PCKeyboard.cpp
      Note: Arduino keyboard I2C client library
    - Path: arduino_picocalc_kbd/src/PCKeyboard.h
      Note: Keyboard API (attachInterrupt
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Main source — console
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# PicoCalc Console Architecture

## Goal

A deep technical analysis of every layer in the uLisp PicoCalc console system — display pipeline, keyboard routing, interrupt mechanics, bus topology, and RP2040 hardware capabilities — to inform the design of a tiled window manager.

Think of this as the "what exists now" document. The companion "what to build" document will come later, referencing specific sections here as constraints.


---

## 1. The Big Picture

The uLisp PicoCalc port has three hardware subsystems talking to the RP2040:

```
                    RP2040 (Core 0 only, Core 1 idle)
                    ┌──────────────────────────────────────┐
                    │                                      │
  Keyboard ──I2C1──│ GP6 (SDA)   GP10 (SCK)  ──SPI1──────│── ILI9488 Display
  (STM32)    10kHz │ GP7 (SCL)   GP11 (MOSI)              │    (320×320)
                    │              GP12 (MISO)              │
                    │              GP13 (CS)                │
                    │              GP14 (DC)                │
                    │              GP15 (RST)               │
                    │                                      │
  USB Serial───────│ GP0 (TX)    GP16 (MISO) ──SPI0───────│── SD Card
  (Serial1)        │ GP1 (RX)    GP17 (CS)                │
                    │              GP18 (SCK)               │
                    │              GP19 (MOSI)              │
                    │                                      │
                    │ GP26 (Audio L)  GP27 (Audio R)       │── Speaker (PWM)
                    └──────────────────────────────────────┘
```

**Key observation:** The display is on SPI1, the SD card is on SPI0, and the keyboard is on I2C1. These are three independent buses. There are no bus collisions. You can talk to the display while reading the keyboard without any contention.

**Core 1 is completely unused.** The entire uLisp interpreter, display driver, and keyboard polling all run on Core 0. Core 1 is available for background rendering, input handling, or a window manager compositor.

---

## 2. The Display Pipeline

Characters travel from the Lisp interpreter to the screen through this chain:

```
Lisp eval/print
      │
      ▼
  pserial(c)           ← the "print to serial" function
      │
      ├─→ Display(c)   ← character-by-character console renderer
      │      │
      │      ├─→ PlotChar(ch, line, col)   ← writes to ScrollBuf + tft.drawChar()
      │      │
      │      └─→ ScrollDisplay()           ← scroll up one line
      │
      └─→ Serial.write(c)  ← (only if serialmonitor #defined)
```

Every character the interpreter outputs goes through `pserial()`. This function does two things: echoes the character to the PicoCalc display via `Display()`, and (optionally) writes it to the USB serial port.


### 2.1 The Scroll Buffer

The console is backed by a two-dimensional character array:

```c
// Lines 7211-7217 of ulisp-picocalc.ino
const int ScreenWidth = 320, ScreenHeight = 320;
const int CharWidth = 6, CharHeight = 8, Leading = 10;
const int Columns = 53;                    // 320 / 6 = 53 columns
const int Lines = 32;                      // 320 / 10 = 32 lines
char ScrollBuf[Columns][Lines];            // 53 × 32 = 1,696 bytes
uint8_t Scroll = 0;                        // circular buffer offset
```

The buffer stores what character is at each `(column, line)` position. The `Scroll` variable implements a circular buffer: when the display scrolls, `Scroll` increments and the oldest row is overwritten. Each cell stores a raw `char` — the high bit (0x80) is repurposed as an "invert" flag for parenthesis highlighting.

**Physical layout on screen:**

```
Line 0  ┌─────────────────────────────────────────────────┐
Line 1  │ uLisp 4.8f                                       │
Line 2  │ 22801> (+ 1 2 3)                                 │
...     │ 6                                                  │
Line 31 │ > _                                               │
        └─────────────────────────────────────────────────┘
        320 pixels (53 chars × 6px each = 318px, 2px unused)
        320 pixels (32 lines × 10px each = 320px exactly)
```

### 2.2 PlotChar — Drawing a Single Character

```c
void PlotChar(uint8_t ch, uint8_t line, uint8_t column) {
  uint16_t y = line * Leading;        // pixel Y
  uint16_t x = column * CharWidth;    // pixel X
  ScrollBuf[column][(line+Scroll) % Lines] = ch;   // update buffer
  if (ch & 0x80) {
    tft.drawChar(x, y, ch & 0x7f, TFT_BLACK, TFT_GREEN, 1);  // inverted
  } else {
    tft.drawChar(x, y, ch & 0x7f, TFT_WHITE, TFT_BLACK, 1);  // normal
  }
}
```

Each `PlotChar` call does exactly one SPI transaction to the ILI9488: set address window to a 6×8 pixel rectangle, then write 48 pixels of glyph data. At 25MHz SPI, this takes roughly 2-3 microseconds per character.

The high bit trick means parenthesis can be highlighted in green-on-black without needing a separate color attribute array. The `Display()` function uses control codes 17-20 for this:

```
char 17 → PlotChar('(' )          // normal open paren
char 18 → PlotChar('(' | 0x80)    // highlighted open paren (green)
char 19 → PlotChar(')' )          // normal close paren
char 20 → PlotChar(')' | 0x80)    // highlighted close paren (green)
```

### 2.3 ScrollDisplay — The Expensive Operation

When the cursor reaches the bottom line and a newline is printed, `ScrollDisplay()` fires:

```c
void ScrollDisplay() {
  // 1. Clear the bottom line
  tft.fillRect(0, ScreenHeight-Leading, ScreenWidth, Leading, TFT_BLACK);

  // 2. For every column, check each row against the row below it
  for (uint8_t x = 0; x < Columns; x++) {
    char c = ScrollBuf[x][Scroll];           // top row (being discarded)
    for (uint8_t y = 0; y < Lines-1; y++) {  // 31 rows to check
      char c2 = ScrollBuf[x][(y+Scroll+1) % Lines];  // row below
      if (c != c2) {                          // only redraw if different
        // draw c2 at position (x, y)
        tft.drawChar(x*CharWidth, y*Leading, c2 & 0x7f, fg, bg, 1);
        c = c2;
      }
    }
  }

  // 3. Clear the old top row in the buffer
  for (int x=0; x < Columns; x++) ScrollBuf[x][Scroll] = 0;
  Scroll = (Scroll + 1) % Lines;
}
```

**Performance characteristics:**

- Worst case: 53 × 31 = 1,643 SPI character writes per scroll (every character differs)
- Best case: 1 fillRect + 53 zero-writes (nothing changed)
- In practice, scrolling a full REPL output triggers ~500-1000 character redraws
- Each `drawChar` is a 6×8 pixel SPI write at 25MHz ≈ 2µs, so worst case ≈ 3-5ms per scroll

**The tidying hack:** After redrawing characters, the function does two fill operations:

```c
for (y = 0; y < Lines-1; y++) tft.fillRect(0, y*Leading+8, 320, Leading-8, TFT_BLACK);
tft.fillRect(TextWidth, 0, ScreenWidth-TextWidth, ScreenHeight, TFT_BLACK);
```

These clear the 2-pixel gap between character rows and the 2-pixel right margin. Without this, glyph rendering artifacts from graphics operations would remain visible. Each `fillRect` is a full-width SPI write, so this adds ~32 + 1 = 33 rectangle clears per scroll.

**Implication for a window manager:** If each window has its own scroll buffer and viewport region, you can scroll individual windows by redrawing only the characters within that window's bounds. The cost is proportional to window size, not full screen.


### 2.4 Display() — The Character State Machine

The `Display(char c)` function is the heart of the console. It maintains static `line` and `column` cursor positions and interprets control characters:

```
Input character c
      │
      ├─ 0x08 (BS)          → move cursor back one position
      ├─ 0x09 (HT)          → move cursor forward one position
      ├─ 0x07 (BEL)         → play a short beep tone
      ├─ 0x0C (FF)          → clear screen, reset cursor to (0,0)
      ├─ 0x0A (LF)          → newline: column=0, advance line or scroll
      ├─ 0x0B (VT)          → jump to bottom-2 line (used by screen editor)
      ├─ 0x7F (DEL)         → move cursor back (same as BS)
      ├─ 17-20              → draw parenthesis (normal or highlighted)
      ├─ ≥ 32 (printable)   → PlotChar, advance column, wrap if needed
      └─ other              → ignored
```

The cursor is drawn as character `_` (0x5F) at the current position. Before processing each character, the old cursor is erased (`PlotChar(' ', line, column)`), and after processing, the new cursor is drawn (`PlotChar(Cursor, line, column)`). This means every character output generates two SPI writes: one erase and one new character (plus the cursor draw).

**Control characters the window manager can intercept:**

| Code | Name | Current behavior | WM opportunity |
|------|------|-----------------|----------------|
| 0x08 | BS | Move cursor left | Backspace in focused window |
| 0x0C | FF | Clear entire screen | Clear focused window only |
| 0x0A | LF | Newline + scroll | Scroll focused window |
| 0x0B | VT | Jump to bottom | Window-local cursor |
| 0x07 | BEL | Beep | Could flash window border |

**The gfxwrite path:** There is a second output path. When uLisp uses `print` with `gfxwrite` as the print function (via `(with-gfx ...)` or similar), characters go directly to `tft.write(c)` — bypassing the scroll buffer entirely. This is used for graphics text overlays that don't participate in scrolling.

### 2.5 pserial() — The Output Funnel

```c
void pserial(char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) Display(c);   // show on PicoCalc display
  #if defined(serialmonitor)
  if (c == '\n') Serial.write('\r');
  Serial.write(c);                    // echo to USB serial
  #endif
}
```

The `NOECHO` flag suppresses display output during paste operations — when loading a Lisp library from the serial port, the characters are echoed back to serial but not rendered on screen (which would be slow). For a window manager, `pserial` would need to know *which window* to target, or the console state machine would need to be per-window.


---

## 3. The Keyboard Pipeline

Keyboard input flows through a multi-stage pipeline from the physical keys to the Lisp reader:

```
Physical keys (8×7 matrix)
      │
      ▼
  STM32G031 keyboard MCU                   ← separate processor
  │  scans matrix every 16ms (KEY_POLL_TIME)
  │  debounces, tracks hold/release
  │  enqueues into FIFO (size 10)
  │  raises interrupt on INT pin (if configured)
  │
      ▼
  I2C bus (Wire1, 10kHz, address 0x1F)
      │
      ▼
  RP2040 reads FIFO register 0x09
      │
      ├─→ gserial() path  (REPL input)
      │     pc_kbd.keyCount() > 0?
      │       → keyEvent() reads FIFO
      │       → ProcessKey(temp) builds KybdBuf[]
      │       → on Enter: KybdAvailable = 1
      │       → gserial() returns chars from KybdBuf[]
      │
      ├─→ testescape() path  (interrupt during eval)
      │     called every 500ms during long operations
      │     checks for ESC (0xB1) or '~'
      │     → error2("escape!") longjmp back to REPL
      │
      └─→ fn_getkey() path  (Lisp (get-key))
            blocks polling pc_kbd.keyEvent()
            returns character to Lisp
```

### 3.1 The Keyboard MCU (STM32G031)

The keyboard is not a simple matrix — it's a separate microcontroller that handles scanning, debouncing, and key state tracking. It communicates as an I2C slave.

**Key event lifecycle on the STM32:**

1. Physical key pressed → matrix scan detects it
2. Debounce timer (configurable via register 0x06, default ~16ms)
3. After debounce: `KEY_STATE_PRESSED` event enqueued in FIFO (max 10 items)
4. If key held >300ms: `KEY_STATE_HOLD` event enqueued
5. On release: `KEY_STATE_RELEASED` event enqueued

**FIFO format (register 0x09):**

Reading 2 bytes from register 0x09 returns:
```
Byte 0: state (0=idle, 1=pressed, 2=held, 3=released)
Byte 1: key code
```

Key codes follow the PicoCalc convention defined in `keyboard.h`:

| Key | Code | Key | Code | Key | Code |
|-----|------|-----|------|-----|------|
| ESC | 0xB1 | F1-F10 | 0x81-0x90 | Arrow keys | 0xB4-0xB7 |
| Tab | 0x09 | Enter | 0x0A | Backspace | 0x08 |
| DEL | 0xD4 | Home | 0xD2 | End | 0xD5 |
| PgUp | 0xD6 | PgDn | 0xD7 | Insert | 0xD1 |
| ALT | 0xA1 | Shift L/R | 0xA2/0xA3 | CTRL | 0xA5 |
| SYM | 0xA4 | CapsLock | 0xC1 | Power | 0x91 |

Regular ASCII characters (a-z, 0-9, symbols) come through as their normal ASCII codes.

**The STM32 can signal interrupts.** Register 0x02 (CFG) has bits to enable interrupts for:
- `CFG_KEY_INT` (bit 4) — key event available
- `CFG_OVERFLOW_INT` (bit 1) — FIFO overflow
- `CFG_CAPSLOCK_INT` (bit 2) — caps lock changed
- `CFG_NUMLOCK_INT` (bit 3) — num lock changed
- `CFG_PANIC_INT` (bit 5) — panic

When an interrupt fires, the STM32 pulses the INT pin (which GPIO pin depends on wiring — the example uses pin 5). The RP2040 can attach a GPIO interrupt to this pin for event-driven keyboard reading instead of polling.

### 3.2 The I2C Bus — Speed Constraint

The keyboard I2C bus runs at **10kHz**. This is extremely slow by I2C standards (standard mode is 100kHz, fast mode 400kHz). The reason is documented in the PicoCalc firmware:

```c
#define I2C_KBD_SPEED 10000 // if dual i2c, then the speed of keyboard i2c should be 10khz
```

This means every I2C transaction to the keyboard takes significant time:
- Write register address: ~1 byte at 10kHz ≈ 0.8ms (with overhead ~1.5ms)
- Read 2 bytes: ~2 bytes at 10kHz ≈ 1.6ms (with overhead ~2.5ms)
- Total FIFO read: **~4ms per key event**

For a window manager, this means keyboard polling cannot be done at high frequency. At best, you can read ~250 keys/second over this bus. The solution is to use the STM32's interrupt pin — only read the FIFO when notified that keys are available.

### 3.3 gserial() — The REPL Input Function

The `gserial()` function is the keyboard reader for the REPL. It has two modes:

**Mode 1: Waiting for input (`KybdAvailable == 0`)**
```c
while (!KybdAvailable) {
  // Check USB serial first (if serialmonitor defined)
  if (Serial.available()) return Serial.read();

  // Poll keyboard
  if (pc_kbd.keyCount() > 0) {
    KeyEvent key = pc_kbd.keyEvent();
    if (key.state == StatePress) {
      if (key.key == '\t') autoComplete();     // Tab → autocomplete
      else ProcessKey(key.key);                 // Feed to line editor
    }
  }
}
```

This is a **blocking poll loop.** The CPU spins checking keyboard and serial. No other code runs during this time. For a window manager, this must change — you need cooperative or preemptive multitasking.

**Mode 2: Returning characters (`KybdAvailable == 1`)**
```c
if (ReadPtr != WritePtr) return KybdBuf[ReadPtr++];
KybdAvailable = 0;
WritePtr = 0;
return '\n';
```

After the user presses Enter, the REPL reads characters one by one from `KybdBuf[]`. This is synchronous — the Lisp reader calls `gserial()` repeatedly to get the next character of the expression.

### 3.4 ProcessKey() — The Line Editor

`ProcessKey(char c)` implements a mini line editor with these features:

- **Character buffering:** Appends to `KybdBuf[]` up to `KybdBufSize` (53×32 = 1,696 chars)
- **Backspace (0x08):** Decrements write pointer, moves cursor back
- **Enter (0x0A/0x0D):** Sends newline to display, sets `KybdAvailable = 1`, signals gserial to start returning characters
- **Escape (0xB1):** Sets `ESCAPE` flag — causes the next `testescape()` to abort
- **Shift+Return (0xD1 = 209):** Replays the previous input (`LastWritePtr` chars)
- **Tab:** Triggers autocomplete (scans keyword table for prefix matches)
- **Parenthesis highlighting:** On `)`, scans backward for matching `(` and highlights both
- **String tracking:** Knows when cursor is inside a string literal (disables paren matching)

**The autocomplete system** is surprisingly sophisticated. It scans backward from the cursor to find the current word prefix, then cycles through all matching uLisp built-in function names on each Tab press.

**Shift+Return** is a line recall feature — it re-enters the previous expression. This is the only "history" mechanism.


---

## 4. Interrupt and Escape Handling

### 4.1 testescape() — Cooperative Interrupts

The Lisp interpreter is fundamentally single-threaded and blocking. Long-running computations (like computing `(fib 30)`) would be uninterruptible if not for `testescape()`:

```c
void testescape() {
  static unsigned long n;
  if (millis() - n < 500) return;        // throttle to every 500ms
  n = millis();

  // Check USB serial for '~' character
  if (Serial.available() && Serial.read() == '~') error2("escape!");

  // Poll keyboard for ESC or '~'
  if (pc_kbd.keyCount() > 0) {
    KeyEvent key = pc_kbd.keyEvent();
    if (key.state == StatePress) {
      char c = key.key;
      if (c == '~' || c == KEY_ESC) error2("escape!");
    }
  }
}
```

`testescape()` is called from strategic points in the interpreter:
- `printarray()` — while printing long arrays
- `apropos()` — while scanning symbol table
- `fn_dotimes()`, `fn_for`, `sp_formillis` — loop bodies
- `eval()` — the main evaluation loop
- `serial1read()`, `serial2read()` — while waiting for serial data
- `readbitarray()` — while reading bit arrays

It works by calling `error2("escape!")`, which does a `longjmp` back to the error handler in `loop()`. This is a **non-local goto** — it unwinds the entire C stack, discarding all local state. For a window manager, this means ESC must be handled carefully: aborting one window's computation should not crash another window's state.

### 4.2 The Flag System

uLisp uses a bitmap flag register for cooperative state:

```c
enum flag {
  PRINTREADABLY,   // Print with escaping (strings quoted, etc.)
  RETURNFLAG,      // Signal return from block
  ESCAPE,          // User pressed Escape
  EXITEDITOR,      // Exit the screen editor
  LIBRARYLOADED,   // Lisp library already loaded
  NOESC,           // Disable escape checking (for #. reader macro)
  NOECHO,          // Don't echo to display (during paste)
  MUFFLEERRORS,    // Suppress error messages
  BACKTRACE        // Show backtrace on error
};
```

The `NOESC` flag is interesting: when the `#.` reader macro evaluates code at read time, escape checking is disabled. This prevents a long-running `#.` expression from being interrupted. For a window manager, you'd want per-window escape flags.

### 4.3 Current Interrupt Limitations

**No true interrupts are used.** The RP2040 has rich interrupt capabilities (GPIO interrupts, timer alarms, DMA completion), but uLisp uses none of them:

- Keyboard: polled in `gserial()` and `testescape()`
- Display: synchronous SPI writes (blocking)
- Timer: only `millis()` for throttling and delay
- No DMA for SPI transfers
- No alarm/callback timers
- Core 1 completely idle

This is a clean slate for the window manager — there are no existing interrupt handlers to conflict with.

---

## 5. Bus Topology and Pin Assignment

Complete RP2040 pin usage in the uLisp PicoCalc port:

```
GP0  ─ UART0 TX (Serial1, USB serial)     │  GP16 ─ SPI0 MISO (SD card)
GP1  ─ UART0 RX (Serial1, USB serial)     │  GP17 ─ SPI0 CS   (SD card)
GP2  ─ (available)                         │  GP18 ─ SPI0 SCK  (SD card)
GP3  ─ (available)                         │  GP19 ─ SPI0 MOSI (SD card)
GP4  ─ (available)                         │  GP20 ─ (available)
GP5  ─ (available)                         │  GP21 ─ (available)
GP6  ─ I2C1 SDA (keyboard)                │  GP22 ─ (available)
GP7  ─ I2C1 SCL (keyboard)                │  GP23 ─ PICO_PS (power save)
GP8  ─ (available)                         │  GP24 ─ (available)
GP9  ─ (available)                         │  GP25 ─ (LED on Pico board)
GP10 ─ SPI1 SCK  (display)                │  GP26 ─ PWM Audio L
GP11 ─ SPI1 MOSI (display)                │  GP27 ─ PWM Audio R
GP12 ─ SPI1 MISO (display)                │  GP28 ─ (available, ADC2)
GP13 ─ SPI1 CS   (display)                │  GP29 ─ (available, ADC3)
GP14 ─ SPI1 DC   (display)                │
GP15 ─ SPI1 RST  (display)                │
```

**Available pins for window manager use:**
- GP2, GP3, GP4, GP5, GP8, GP9, GP20, GP21 — general purpose I/O
- GP28, GP29 — analog capable (ADC)

If the keyboard interrupt pin from the STM32 is wired to one of the available GPIO pins (the library example uses pin 5), the RP2040 can attach a GPIO interrupt for event-driven keyboard reading. You would configure the STM32's CFG register to enable `CFG_KEY_INT`, then use `attachInterrupt()` on the RP2040 side.


---

## 6. Graphics Functions (Coexisting with Console)

The uLisp PicoCalc port has a full set of graphics primitives that write directly to the display via TFT_eSPI, bypassing the console scroll buffer:

```c
// Drawing primitives (lines 5260-5465)
tft.fillRect(x, y, w, h, color);         // (fill-rect x y w h color)
tft.drawPixel(x, y, color);              // (draw-pixel x y color)
tft.drawFastHLine(x, y, w, color);       // (draw-line x y w color) horizontal
tft.drawFastVLine(x, y, h, color);       // (draw-line x y h color) vertical
tft.fillCircle(x, y, r, color);          // (fill-circle x y r color)
tft.fillRoundRect(x, y, w, h, r, color); // (fill-roundrect ...)
tft.fillTriangle(x1,y1,x2,y2,x3,y3,c);  // (fill-triangle ...)
tft.drawBitmap(x, y, bitmap, w, h, c);   // bitmap drawing

// Text on graphics
tft.setCursor(x, y);                     // (set-cursor x y)
tft.setTextColor(color);                 // (set-text-color color)
tft.setTextColor(fg, bg);                // (set-text-color fg bg)
tft.setTextSize(size);                   // (set-text-size size)
tft.setTextWrap(bool);                   // (set-text-wrap flag)
tft.fillScreen(color);                   // (fill-screen color)
tft.setRotation(n);                      // (set-rotation n)
```

These all use the TFT_eSPI global `tft` object. They write directly to the display — no scroll buffer, no character grid. This means **graphics and console share the same framebuffer** (the ILI9488's internal display RAM). There is no layering.

**Implication for window manager:** When a window is in "graphics mode," the WM must track which screen regions are owned by graphics and exclude them from console scrolling. Alternatively, each window gets its own virtual framebuffer and the WM composites them.

### 6.1 GFXSTREAM — Dual Output

uLisp defines a `GFXSTREAM` output mode:

```c
enum stream { SERIALSTREAM, I2CSTREAM, SPISTREAM, SDSTREAM, WIFISTREAM, STRINGSTREAM, GFXSTREAM };
void gfxwrite(char c) { tft.write(c); }
```

When `print` uses `gfxwrite` as the output function, characters go directly to `tft.write()` — the TFT_eSPI `Print` interface. This writes characters at the current graphics cursor position, using the current graphics text settings. It does not update the console scroll buffer.

The pretty-printer adjusts its width when outputting to GFXSTREAM:
```c
const int PPWIDTH = 52;          // serial pretty-print width
const int GFXPPWIDTH = 52;       // graphics pretty-print width (320/6 = 53)
int ppwidth = PPWIDTH;
```

---

## 7. The REPL Loop — Where Everything Connects

The main loop ties all systems together:

```c
void loop() {
  if (!setjmp(toplevel_handler)) {
    autorunimage();    // load saved image if autorun is set
  }
  ulisperror();        // error cleanup (reset state, flush serial)
  repl(NULL);          // enter read-eval-print loop
}

void repl(object *env) {
  for (;;) {
    randomSeed(micros());
    gc(NULL, env);               // print free space
    pserial('>'); pserial(' ');  // print prompt
    object *line = readmain(gserial);  // READ: blocks on keyboard
    line = eval(line, env);            // EVAL: runs Lisp code
    printobject(line, pserial);        // PRINT: output result
    pln(pserial);                      // newline
  }
}
```

The flow is strictly synchronous:

```
loop() → repl()
           │
           ├─ print prompt → pserial() → Display()
           │
           ├─ readmain(gserial)    ← BLOCKS here until Enter
           │     │                     polls keyboard, builds line
           │     └─ returns Lisp form
           │
           ├─ eval(form, env)      ← may call testescape()
           │     │                     may call pserial() for trace output
           │     └─ returns result
           │
           └─ printobject(result)  ← pserial() → Display()
                 │
                 └─ loop back to prompt
```

**There is no concurrency.** While the REPL is waiting for input (`gserial()`), the CPU spins polling the keyboard. While evaluating, `testescape()` polls the keyboard every 500ms. While printing results, the keyboard is not checked at all.

**For a window manager, this architecture must change fundamentally.** You need either:
1. **Cooperative multitasking:** The Lisp evaluator yields periodically to let the WM process keyboard events for other windows
2. **Core 1 compositing:** Run the window manager on Core 1, with the Lisp REPL on Core 0 communicating via the RP2040's inter-core FIFO
3. **Interrupt-driven input:** Use GPIO interrupt for keyboard, timer interrupt for WM refresh


---

## 8. RP2040 Dual-Core Capabilities

The RP2040 has two identical Cortex-M0+ cores. Currently, uLisp uses only Core 0. Core 1 is completely idle and available.

### 8.1 Inter-Core Communication

The RP2040 provides a hardware FIFO for inter-core communication:

```c
#include "pico/multicore.h"

// Core 0 pushes data for Core 1
multicore_fifo_push_blocking(uint32_t data);

// Core 1 reads data from Core 0
uint32_t data = multicore_fifo_pop_blocking();

// Check if data available
bool multicore_fifo_rvalid();   // Core 0 can read?
bool multicore_fifo_wready();   // Core 0 can write?
```

The FIFO is 8 entries deep, with separate read/write_valid signals for each core. Each entry is 32 bits. There are also per-core interrupt lines that fire when the FIFO has data or becomes empty.

### 8.2 Proposed Dual-Core Architecture

```
Core 0 (Lisp interpreter)              Core 1 (Window Manager)
┌─────────────────────────┐            ┌─────────────────────────┐
│  repl()                  │            │  Main loop:              │
│    ├─ gserial() ─────────│── FIFO ──>│    poll keyboard (I2C)   │
│    ├─ eval()             │            │    route key to window   │
│    └─ pserial() ─────────│── FIFO ──>│    composite windows     │
│                          │            │    render dirty regions  │
│  pserial reads from:     │            │    handle scroll/resize  │
│  <───────────────────────│── FIFO ───│    forward keys to Core0 │
│                          │            │                          │
│  testescape() ───────────│── FIFO ──>│    check for ESC         │
└─────────────────────────┘            └─────────────────────────┘
                                          │
                                          ▼
                                       SPI1 → ILI9488 Display
```

**Core 1 owns the display and keyboard.** Core 0 runs the Lisp interpreter as before, but:
- `pserial()` pushes characters into the FIFO instead of writing to the display directly
- `gserial()` reads characters from the FIFO instead of polling the keyboard
- `testescape()` checks a shared flag set by Core 1 when ESC is detected

**Core 1's event loop:**
```
while (true) {
  if (keyboard_interrupt or keyboard_poll) {
    read key from FIFO register 0x09
    if key is window-management key (Alt+Tab, etc.):
      handle window switch
    else:
      push key into Core 0 FIFO for gserial()
  }
  if (Core 0 has output in FIFO):
    read character
    write to focused window's virtual console
    if window is dirty:
      redraw dirty region via SPI
  if (timer tick):
    redraw any pending window borders/overlays
}
```

### 8.3 Memory Sharing

Both cores share the same address space. The RP2040 has 264KB of SRAM (six banks of 4KB each). Both cores can read/write any address. Synchronization is via the hardware spinlocks (8 available) or the FIFO.

For a window manager:
- Virtual console buffers can live in shared SRAM
- Core 0 writes characters into the virtual console
- Core 1 reads from virtual consoles and renders to the display
- A dirty-flag per window line tells Core 1 what needs redrawing

**Memory budget for virtual consoles:**

The current scroll buffer is 53×32 = 1,696 bytes per console. If each window has its own:
- 4 windows × 1,696 bytes = 6,784 bytes
- Add color attributes (1 byte per char): 4 × 1,696 = 6,784 more bytes
- Total: ~14KB for 4 window buffers

The RP2040 has ~200KB available (after uLisp workspace and stack), so this is easily affordable.

---

## 9. Window Manager Design Constraints

Based on the architecture analysis, here are the hard constraints and opportunities for a tiled window manager:

### Hard Constraints

| Constraint | Value | Impact |
|-----------|-------|--------|
| Display resolution | 320×320 pixels | Very small — at 6×8 char size, only 53×32 characters fit. Splitting into 2 or 4 windows leaves tiny viewports. |
| Display SPI speed | 25 MHz | Full screen redraw: 320×320×2 bytes = 204,800 bytes → ~8ms. Partial redraw of a quarter window: ~2ms. |
| Keyboard I2C speed | 10 kHz | ~4ms per key read. Must use interrupt pin or limit poll rate. |
| RAM available | ~200 KB (after uLisp) | Plenty for virtual consoles, window structures, and buffers. |
| Single-threaded REPL | Blocking gserial() | Must break the blocking poll loop or move keyboard handling to Core 1. |
| No OS, no threads | Bare metal | Cooperative or interrupt-driven multitasking only. |

### Opportunities

| Opportunity | How |
|------------|-----|
| Core 1 is free | Run WM compositor and keyboard router on Core 1 |
| GPIO interrupts available | Keyboard STM32 can drive INT pin → RP2040 GPIO interrupt → event-driven input |
| RP2040 hardware timers | 4 alarm slots for periodic WM refresh, blink cursor, etc. |
| SPI DMA | RP2040 DMA can feed SPI without CPU involvement — fast window redraw |
| Separate SPI buses | Display and SD card don't interfere — can load files while rendering |
| 8 hardware spinlocks | Lock-free coordination between Core 0 and Core 1 |
| Plenty of free GPIO pins | Can add hardware buttons for window management shortcuts |

### Design Questions for Next Step

1. **Tile layout:** Fixed 2×2 grid? Horizontal split? User-controllable splits (like i3wm)?
2. **Window focus model:** Click-to-focus (impossible without touch/mouse) or keyboard-driven (Alt+Tab, arrow keys)?
3. **Per-window console:** One `ScrollBuf` per window, or one shared with viewport offsets?
4. **Graphics windows:** Can a window be in "graphics mode" while others are text? How to clip?
5. **Core split:** What exactly runs on Core 0 vs Core 1?
6. **Escape handling:** Per-window ESC, or global? How does abort work in background windows?

---

## 10. Reference: Key Source Locations

| What | File | Lines |
|------|------|-------|
| Console constants | `ulisp-picocalc.ino` | 7211-7218 |
| PlotChar | `ulisp-picocalc.ino` | 7224-7232 |
| ScrollDisplay | `ulisp-picocalc.ino` | 7234-7256 |
| Display() state machine | `ulisp-picocalc.ino` | 7258-7300 |
| pserial() | `ulisp-picocalc.ino` | 6976-6982 |
| gserial() | `ulisp-picocalc.ino` | 7443-7478 |
| ProcessKey() | `ulisp-picocalc.ino` | 7387-7425 |
| testescape() | `ulisp-picocalc.ino` | 6742-6754 |
| getKey() / fn_getkey | `ulisp-picocalc.ino` | 5478-5497 |
| autoComplete() | `ulisp-picocalc.ino` | 7316-7362 |
| initgfx() / setup() | `ulisp-picocalc.ino` | 7530-7545 |
| repl() / loop() | `ulisp-picocalc.ino` | 7548-7580 |
| gfxwrite() | `ulisp-picocalc.ino` | 2293 |
| Graphics functions | `ulisp-picocalc.ino` | 5260-5465 |
| TFT pin config | `Setup60_RP2040_ILI9488.h` | all |
| Keyboard library | `PCKeyboard.cpp` | all |
| STM32 keyboard firmware | `picocalc_keyboard.ino` | all |
| RP2040 I2C keyboard client | `i2ckbd.c/h` | all |
| PicoCalc booter LCD driver | `lcdspi.h` | all |
| PicoCalc pin config | `config.h` | all |
| Interrupt-driven example | `InterruptDriven.ino` | all |
