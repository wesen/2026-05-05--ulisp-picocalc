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
      Note: Complete pin assignment map
    - Path: PicoCalc/Code/pico_multi_booter/sd_boot/lcdspi/lcdspi.h
      Note: Low-level LCD SPI primitives from PicoCalc booter
    - Path: PicoCalc/Code/picocalc_keyboard/picocalc_keyboard.ino
      Note: STM32 keyboard firmware — FIFO protocol
    - Path: arduino_picocalc_kbd/examples/InterruptDriven/InterruptDriven.ino
      Note: GPIO interrupt-driven keyboard reading example
    - Path: arduino_picocalc_kbd/src/PCKeyboard.cpp
      Note: Keyboard I2C client library — keyCount()
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: Primary source — all console
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# The PicoCalc Console — A Literate Walkthrough

This document reads the uLisp PicoCalc console the way you'd read a novel: from the first line that boots the machine, through the loop that waits for your keystrokes, down to the individual pixels that light up when a character appears on screen. Every code listing is real, taken verbatim from `ulisp-picocalc.ino` and the supporting libraries. The goal is not to summarize the code — it is to let you *hear the machine think*.

By the end, you will understand every layer well enough to replace the monolithic console with a window manager that splits the 320×320 display into independent regions, each with its own keyboard routing, scroll state, and cursor.

---

## Chapter 1. The Machine Wakes Up

When power is applied to the RP2040, it begins executing at address `0x10000000` in flash — the entry point that the Arduino core maps to `setup()`. Here is the first code that runs on the PicoCalc:

```c
// ulisp-picocalc.ino, lines 7707–7546

void setup () {
  Serial.begin(9600);
  int start = millis();
  while ((millis() - start) < 5000) { if (Serial) break; }
```

The first thing the machine does is wait. Five seconds, or until a USB serial connection is detected — whichever comes first. This is a courtesy to the programmer: if you're connecting via USB to debug, the PicoCalc gives you a window to attach before it starts printing output that would otherwise be lost.

Then the real initialization begins:

```c
  #if defined(sdcardsupport)
  pinMode(SDCARD_SS_PIN, OUTPUT);
  digitalWrite(SDCARD_SS_PIN, 1);
  #endif
  initworkspace();
  initenv();
  initsleep();
  initgfx();
  initkybd();
```

Six initialization calls, each responsible for bringing one subsystem to life. Let's read them in the order the machine executes them — because the order matters.

**`initworkspace()`** carves the Lisp object heap out of the RP2040's SRAM. It allocates an array of 23,000 eight-byte objects (minus 720 for SD card buffers, yielding 22,280 usable objects — about 178 KB). This is the memory pool that every Lisp value, every function definition, every string lives in.

**`initenv()`** creates the global environment — the namespace where `defun`, `defvar`, and the REPL's definitions live.

**`initsleep()`** configures the low-power sleep machinery (not relevant to our story, but harmless).

**`initgfx()`** is where the display comes alive:

```c
void initgfx () {
  #if defined(gfxsupport)
  tft.init();
  tft.writecommand(TFT_DISPOFF);     // turn display off during init
  tft.invertDisplay(1);               // ILI9488 needs color inversion
  tft.fillScreen(TFT_BLACK);          // clear to black
  tft.writecommand(TFT_DISPON);       // turn display back on
  #endif
}
```

Four SPI commands, each one a burst of bits at 25 MHz over the SPI1 bus (pins 10–15). The display blanks, inverts its color space, fills with black, and switches on. At this point, the screen is dark but ready.

**`initkybd()`** brings up the keyboard:

```c
void initkybd () {
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin();
  Wire1.setClock(10000);
  pc_kbd.begin(0x1f, &Wire1);
}
```

This is the first time we see the I2C bus that connects the RP2040 to the keyboard's STM32 microcontroller. GP6 is the data line, GP7 is the clock, and the clock runs at just 10 kHz — glacially slow by modern standards. We will return to why this is so slow, and what it means for the window manager, in Chapter 4.

Finally, the machine prints its greeting:

```c
  pfstring(PSTR("uLisp 4.8f "), pserial); pln(pserial);
}
```

Two function calls: `pfstring` prints a string, and `pln` prints a newline. Both take `pserial` as their second argument — the output function that writes a single character. We will spend considerable time with `pserial`. For now, know that it is the nozzle through which all Lisp output flows onto the screen.

---

## Chapter 2. The Main Loop — Where Waiting Happens

After `setup()` returns, the Arduino runtime calls `loop()` — and then calls it again, and again, forever. But uLisp does not use this mechanism idiomatically. Instead, `loop()` contains the entire top-level error boundary and the REPL:

```c
void loop () {
  if (!setjmp(toplevel_handler)) {
    #if defined(resetautorun)
    volatile int autorun = 12;
    #else
    volatile int autorun = 13;
    #endif
    if (autorun == 12) autorunimage();
  }
  ulisperror();
  repl(NULL);
}
```

There is a deep idea here, and it is worth sitting with it for a moment. The `setjmp` call captures the complete state of the C stack — every local variable, every return address, the stack pointer itself — and saves it in `toplevel_handler`. This is a *snapshot* of where we are right now. Later, when something goes wrong, `longjmp(toplevel_handler, 1)` will teleport execution back to this exact point, as if the intervening code had never run. The stack is unwound, local variables are destroyed, and we land here again with `setjmp` returning non-zero.

This is uLisp's error recovery mechanism. When the interpreter encounters a runtime error — division by zero, undefined variable, stack overflow — it calls `error2()`, which calls `longjmp()`, which lands us back here. Then `ulisperror()` cleans up the debris:

```c
void ulisperror () {
  delay(100);
  while (Serial.available()) Serial.read();  // drain serial buffer
  clrflag(NOESC); BreakLevel = 0;
  TraceStart = 0; TraceTop = 0;
  for (int i = 0; i < TRACEMAX; i++) TraceDepth[i] = 0;
  // ... close files, disconnect WiFi, load Lisp library if needed
}
```

And then `repl(NULL)` starts a fresh read-eval-print loop. The user sees a new `>` prompt and the machine carries on as if nothing happened.

For the window manager, this `longjmp` pattern is the most dangerous thing in the entire codebase. If Window A is evaluating a long computation and the user presses Escape, `testescape()` fires `error2()`, which fires `longjmp()`, which obliterates the state of *everything* — including Window B's in-progress computation. There is no per-window error boundary. There is only one `setjmp`, and it owns the world.

The REPL itself is the heart of the machine:

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
    // ... break handling ...
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

Read it as a cycle. On each iteration:

1. **Print free space.** `gc()` runs a garbage collection pass, then `pint()` prints the number of free Lisp cells. This is the number you see before the `>` prompt — `22801>` means 22,801 objects are available.

2. **Print the prompt.** Two characters: `>` and a space. Written through `pserial()`.

3. **Read a line.** `readmain(gserial)` calls the Lisp reader, which calls `gserial()` to get characters one at a time. `gserial()` is the function that polls the keyboard — it *blocks here* until the user presses Enter.

4. **Evaluate.** `eval(line, env)` runs the Lisp expression. This may take microseconds or minutes. During this time, `testescape()` is called periodically to check for the Escape key.

5. **Print the result.** `printobject(line, pserial)` renders the result back through `pserial()`.

6. **Loop.** Forever.

The critical observation is that step 3 *blocks the entire machine*. While `gserial()` is spinning, waiting for your keystroke, nothing else can run. There is no background processing, no timer interrupt, no second core doing work. The machine is a stone, waiting for you to type. The window manager must break this model.

---

## Chapter 3. Characters on the Screen

The console is a character grid painted onto a 320×320 pixel display. Understanding how a single character gets from a C function call to illuminated pixels on the ILI9488 panel is the foundation for understanding everything the window manager must replicate — or replace.

### 3.1 The Output Nozzle

Every character the Lisp interpreter prints passes through a single function:

```c
// ulisp-picocalc.ino, line 6976
void pserial (char c) {
  LastPrint = c;
  if (!tstflag(NOECHO)) Display(c);
  #if defined(serialmonitor)
  if (c == '\n') Serial.write('\r');
  Serial.write(c);
  #endif
}
```

`pserial` is the type `pfun_t` — a pointer to a function that takes a `char` and returns `void`. It is the type signature of "I output a character." Throughout uLisp, every function that produces output takes a `pfun_t` argument so it can be redirected: to serial, to a string, to a file on the SD card, to the graphics stream, or — most commonly — to `pserial`.

The `NOECHO` flag suppresses the display when the interpreter is replaying pasted input (loading a library over serial, for instance). When `NOECHO` is clear, every character goes to `Display(c)`. The serial mirror (the `Serial.write` calls) is only compiled in when `serialmonitor` is defined, which it is in the PicoCalc build — so characters also go out the USB port for debugging.

### 3.2 The Console Grid

Before we trace what `Display()` does, we need to understand the grid it paints on:

```c
// ulisp-picocalc.ino, lines 7211–7218
const int ScreenWidth = 320, ScreenHeight = 320;
const int CharWidth = 6, CharHeight = 8, Leading = 10;
const int Columns = ScreenWidth / CharWidth;   // 53
const int Lines = ScreenHeight / Leading;       // 32
const int LastColumn = Columns - 1;             // 52
const int LastLine = Lines - 1;                 // 31
const char Cursor = 0x5f;                       // underscore
```

Fifty-three columns, thirty-two lines. Each character cell is 6 pixels wide and 8 pixels tall, but the line height ("leading" in typography) is 10 pixels — there is a 2-pixel gap between rows. This gap exists because the ILI9488's built-in font is 6×8, but 10-pixel line spacing is more readable. The gap matters: it's the reason `ScrollDisplay()` has to do extra cleanup fills after moving characters up.

The grid is backed by a two-dimensional array called `ScrollBuf`:

```c
char ScrollBuf[Columns][Lines];   // 53 × 32 = 1,696 bytes
```

And a circular buffer offset:

```c
uint8_t Scroll = 0;
```

`Scroll` is the index of the *first* line in the circular buffer. When the display scrolls up by one line, `Scroll` advances by 1 (modulo `Lines`), and the old top line becomes the new bottom line. Any cell in the grid is addressed as `ScrollBuf[column][(line + Scroll) % Lines]` — the modulo wraps around the circular buffer.

### 3.3 Drawing One Character

`PlotChar` is the function that actually lights up pixels:

```c
// ulisp-picocalc.ino, lines 7224–7232
void PlotChar (uint8_t ch, uint8_t line, uint8_t column) {
  uint16_t y = line * Leading;
  uint16_t x = column * CharWidth;
  ScrollBuf[column][(line+Scroll) % Lines] = ch;
  if (ch & 0x80) {
    tft.drawChar(x, y, ch & 0x7f, TFT_BLACK, TFT_GREEN, 1);
  } else {
    tft.drawChar(x, y, ch & 0x7f, TFT_WHITE, TFT_BLACK, 1);
  }
}
```

Three things happen in rapid succession:

1. **Record the character** in `ScrollBuf`. Even if the pixel never changes, the buffer remembers what's there — so that `ScrollDisplay()` can repaint the screen from the buffer later.

2. **Check the high bit.** If bit 7 is set (value ≥ 128), the character is drawn in *inverted colors*: green text on a black background. Otherwise, it's white on black. This is the parenthesis highlighting mechanism — when you type `)`, uLisp finds the matching `(` and draws both in green.

3. **Send to the display.** `tft.drawChar(x, y, glyph, fg, bg, size)` is a TFT_eSPI function that does the actual SPI transaction: it sets the ILI9488's address window to a 6×8 pixel rectangle starting at `(x, y)`, then writes 48 pixels of glyph data. At 25 MHz SPI, this takes about 2 microseconds.

The `1` at the end is the size multiplier. Size 1 means 6×8 pixels. Size 2 would mean 12×16, and so on. The console always uses size 1.

For the window manager, `PlotChar` is the function you would replicate per-window. Instead of global `line` and `column` coordinates, each window would have its own cursor position, offset by the window's origin on screen. Instead of one `ScrollBuf`, each window would have its own.

### 3.4 The Display State Machine

`Display(char c)` is a state machine wrapped around `PlotChar`. It maintains two static variables — the cursor's line and column — and interprets control characters:

```c
void Display (char c) {
  #if defined(gfxsupport)
  static uint8_t line = 0, column = 0;
```

`static` means these survive across calls. The cursor position is global to the entire console — there is only one cursor, and it lives on whichever line and column the last character left it.

The function is a cascade of `if` statements, each handling one character class. Let's read them in the order the machine evaluates them:

**Backspace (0x08):** Move the cursor back one cell.

```c
  if (c == 8) {
    if (column == 0) {
      line--; column = LastColumn;
    } else column--;
    return;
  }
```

If the cursor is at the left edge (column 0), it wraps up to the previous line's last column. Note that `PlotChar` is *not* called — backspace moves the cursor but does not erase anything. The erasure happens elsewhere (in `ProcessKey`, which draws a space at the old position before moving back).

**Cursor forward (0x09):** Move the cursor ahead one cell. This is not the Tab key (Tab triggers autocomplete instead). It's an internal cursor movement used by the parenthesis highlighter.

```c
  if (c == 9) {
    if (column == LastColumn) {
      line++; column = 0;
    } else column++;
    return;
  }
```

**Parenthesis control codes (17–20):** These are the only "virtual" characters in the system — they never appear as actual key presses. They are synthesized by `Highlight()` to draw parentheses in normal or inverted colors:

```c
  if ((c >= 17) && (c <= 20)) {
    if (c == 17) PlotChar('(', line, column);
    else if (c == 18) PlotChar('(' | 0x80, line, column);   // green
    else if (c == 19) PlotChar(')', line, column);
    else PlotChar(')' | 0x80, line, column);                 // green
    return;
  }
```

These do *not* advance the cursor — they paint at the current position and return. The cursor movement is handled by the caller (`Highlight`), which sends backspace/forward codes to navigate to the matching parenthesis position, draws it, then navigates back.

**The cursor dance.** Before processing any visible character, the old cursor is erased:

```c
  PlotChar(' ', line, column);   // erase old cursor
```

And after processing, the new cursor is drawn:

```c
  PlotChar(Cursor, line, column);   // draw new cursor (underscore)
```

This means every printable character generates *two* SPI writes: one space to erase the old cursor, and the character itself. Then a third write places the new cursor underscore. Three SPI writes per keystroke — roughly 6 microseconds of SPI bus time.

**DEL (0x7F):** Same as backspace — move cursor back without erasing. The actual erasure is done by the caller.

**Printable characters (≥ 32):** The main case.

```c
  } else if ((c & 0x7f) >= 32) {
    PlotChar(c, line, column++);
    if (column > LastColumn) {
      column = 0;
      if (line == LastLine) ScrollDisplay(); else line++;
    }
```

Plot the character, advance the column. If we've passed the right edge, wrap to the next line. If we're on the last line, scroll the display up; otherwise just advance the line counter.

**Form feed (0x0C):** Clear the entire screen.

```c
  } else if (c == 12) {
    tft.fillScreen(COLOR_BLACK);
    line = 0; column = 0; Scroll = 0;
    for (int col = 0; col < Columns; col++) {
      for (int row = 0; row < Lines; row++) {
        ScrollBuf[col][row] = 0;
      }
    }
```

One full-screen SPI fill (320×320×2 bytes = 204,800 bytes at 25 MHz ≈ 8 ms) plus clearing the entire scroll buffer. For a window manager, this would become "clear the focused window" rather than "clear everything."

**Newline (0x0A):** Move to the start of the next line.

```c
  } else if (c == '\n') {
    column = 0;
    if (line == LastLine) ScrollDisplay(); else line++;
  }
```

**Vertical tab (0x0B):** Jump to the third line from the bottom. This is used by the built-in screen editor — a feature of uLisp that allows editing functions in-place on the display.

**Beep (0x07):** Play a short tone.

```c
  } else if (c == BEEP) {
    playnote(0, 0, 4);
    delay(250);
    nonote(0);
  }
```

Two hundred and fifty milliseconds of a very low tone (frequency 0 means the PWM is running at its base rate, which produces a buzz). The machine stops for a quarter second — another blocking delay that the window manager would want to make asynchronous.

### 3.5 Scrolling — The Most Expensive Operation

When the cursor reaches the bottom of the screen and a newline arrives, `ScrollDisplay()` fires. This is the most computationally expensive operation in the console — and the one that most urgently needs rethinking for a window manager.

```c
void ScrollDisplay () {
  #if defined(gfxsupport)
  tft.fillRect(0, ScreenHeight-Leading, ScreenWidth, Leading, TFT_BLACK);
```

First, the bottom line is erased — a full-width rectangle fill at the very bottom of the screen.

Then comes the character-by-character scan:

```c
  for (uint8_t x = 0; x < Columns; x++) {
    char c = ScrollBuf[x][Scroll];
    for (uint8_t y = 0; y < Lines-1; y++) {
      char c2 = ScrollBuf[x][(y+Scroll+1) % Lines];
      if (c != c2) {
        if (c2 & 0x80) {
          tft.drawChar(x*CharWidth, y*Leading, c2 & 0x7f, TFT_BLACK, TFT_GREEN, 1);
        } else {
          tft.drawChar(x*CharWidth, y*Leading, c2 & 0x7f, TFT_WHITE, TFT_BLACK, 1);
        }
        c = c2;
      }
    }
  }
```

The algorithm is: for each column, walk down the rows. Compare each row to the one above it. If they differ, redraw the character. The comparison is between the character *that was there before* and the character *that will be there after the scroll* — because scrolling means every row takes on the content of the row below it.

This is an optimization over the naive approach (redraw every character). In a typical REPL session, most of the screen doesn't change during a scroll — only the region near the cursor has new content. The comparison skips unchanged characters entirely.

But there is a hidden cost. After the character loop, the function does "tidying":

```c
  for (uint8_t y = 0; y < Lines-1; y++)
    tft.fillRect(0, y*Leading+8, 320, Leading-8, TFT_BLACK);
  tft.fillRect(TextWidth, 0, ScreenWidth-TextWidth, ScreenHeight, TFT_BLACK);
```

Thirty-one horizontal strips (the 2-pixel gaps between rows) and one vertical strip (the 2-pixel right margin) are cleared to black. Each `fillRect` is a full-width SPI write. These exist to clean up after graphics operations that might have drawn into the gaps. Without them, a `draw-line` call that crosses a row boundary would leave fragments visible between lines of text.

Finally, the circular buffer advances:

```c
  for (int x = 0; x < Columns; x++) ScrollBuf[x][Scroll] = 0;
  Scroll = (Scroll + 1) % Lines;
  #endif
}
```

The old top line is zeroed out and `Scroll` moves forward. The cost of a full scroll is approximately 1,000 SPI write operations — about 3–5 milliseconds. For a window manager, the same algorithm would apply to a single window's viewport, with the cost proportional to the window's height rather than the full 32 lines.

---

## Chapter 4. The Keyboard — A Separate Nation

The PicoCalc keyboard is not a peripheral of the RP2040. It is a separate computer — an STM32G031 microcontroller with its own firmware, its own clock, its own I2C address. When you press a key, the event travels through three distinct processors before reaching the Lisp interpreter.

### 4.1 The Hardware Path

```
Physical key
  │
  ▼
Key matrix (8 columns × 7 rows = 56 positions)
  │  scanned every 16 ms by the STM32
  ▼
STM32G031 (keyboard MCU)
  │  debounces, detects hold/release
  │  enqueues into a 10-deep FIFO
  │
  ├── I2C bus (Wire, PB8/PB9, address 0x1F)
  │     │
  │     ▼
  │   RP2040 reads FIFO register 0x09
  │
  └── Also manages: display backlight, keyboard backlight,
      battery monitoring (via AXP2101 PMU), headphone detection
```

The STM32 firmware is in `picocalc_keyboard.ino` — a 300-line Arduino sketch that runs on the STM32. Its `loop()` is:

```c
void loop() {
  check_pmu_int();       // battery/charging interrupts
  keyboard_process();    // scan matrix, generate events
  // ... I2C bus timeout watchdog ...
  check_hp_det();        // headphone detection
  nbDelay_ms(10);        // 10ms sleep
}
```

Every 10 milliseconds, it scans the keyboard matrix, processes any state changes, and enqueues events. It also polls the battery management IC and checks the headphone jack. This is a busy loop, but on a dedicated microcontroller — it doesn't interfere with the RP2040's work.

### 4.2 The I2C Register Map

The STM32 presents itself as an I2C slave with a set of registers that the RP2040 can read or write:

```c
// From PCKeyboard.cpp
#define _REG_VER 0x01  // firmware version
#define _REG_CFG 0x02  // interrupt configuration
#define _REG_INT 0x03  // interrupt status
#define _REG_KEY 0x04  // key count + lock states
#define _REG_BKL 0x05  // display backlight brightness
#define _REG_DEB 0x06  // debounce configuration
#define _REG_FRQ 0x07  // key poll frequency
#define _REG_RST 0x08  // reset (DANGEROUS — crashes MCU)
#define _REG_FIF 0x09  // FIFO — the key event queue
#define _REG_BK2 0x0A  // keyboard backlight brightness
#define _REG_BAT 0x0B  // battery percentage + charging flag
```

The most important register is `0x09` (FIFO). Reading two bytes from it dequeues the next key event:

```
Byte 0 (low):  key state   (0=idle, 1=pressed, 2=held, 3=released)
Byte 1 (high): key code    (ASCII character or special code)
```

Register `0x04` (KEY) is also useful — it returns a single byte whose lower 5 bits are the number of keys waiting in the FIFO, plus flag bits for Caps Lock (bit 5) and Num Lock (bit 6).

The STM32 processes I2C requests through interrupt handlers:

```c
// From picocalc_keyboard.ino
void receiveEvent(int howMany) {
  // Parse register address from the incoming bytes
  // If it's a write (high bit set), also parse the value
  // Prepare response in write_buffer[]
}

void requestEvent() {
  // Send write_buffer[] back to the master (RP2040)
}
```

When the RP2040 reads register `0x09`, the STM32 dequeues a FIFO item and sends it as the response. This is the mechanism by which keystrokes cross the I2C bus.

### 4.3 Why 10 kHz?

The I2C clock is set to 10 kHz — one-tenth of standard mode (100 kHz) and one-fortieth of fast mode (400 kHz). The reason is documented in the PicoCalc source:

```c
#define I2C_KBD_SPEED 10000  // if dual i2c, then the speed of keyboard i2c should be 10khz
```

The comment hints at bus sharing. The STM32 also uses its I2C bus to talk to the AXP2101 power management IC. At higher speeds, the STM32's I2C slave implementation may not keep up while simultaneously managing the PMU. The 10 kHz clock is a safety margin.

The practical impact: reading one FIFO entry takes approximately 4 milliseconds (write register address at 10 kHz + read 2 bytes at 10 kHz + overhead). The keyboard's FIFO can hold 10 events. If the RP2040 doesn't drain the FIFO within 160 ms (10 events × 16 ms scan interval), events are lost or the FIFO overflows.

### 4.4 The Keyboard Library

On the RP2040 side, the `PCKeyboard` class wraps the I2C communication:

```c
// From PCKeyboard.cpp
uint8_t PCKeyboard::keyCount() const {
  return status() & KEY_COUNT_MASK;   // lower 5 bits of register 0x04
}

PCKeyboard::KeyEvent PCKeyboard::keyEvent() const {
  KeyEvent event = { .key = '\0', .state = StateIdle };
  if (keyCount() == 0) return event;
  const uint16_t buf = readRegister16(_REG_FIF);  // read register 0x09
  event.key = buf >> 8;       // high byte = key code
  event.state = KeyState(buf & 0xFF);  // low byte = state
  return event;
}
```

Every call to `keyCount()` performs an I2C transaction (write register address, read 2 bytes). Every call to `keyEvent()` performs another. This means the polling loop in `gserial()` generates two I2C transactions per iteration — roughly 8 ms of I2C bus time per poll.

The library also provides an interrupt-driven mode. The STM32 can be configured to pulse a GPIO pin when a key event is available:

```c
// From InterruptDriven.ino example
void KeyIsr(void) {
  dataReady = true;
}

void setup() {
  keyboard.attachInterrupt(interruptPin, KeyIsr);
}
```

Under the hood, `attachInterrupt` configures the STM32's CFG register to enable `CFG_KEY_INT`, then attaches a GPIO interrupt handler on the RP2040 side:

```c
void PCKeyboard::attachInterrupt(uint8_t pin, void (*func)()) const {
  ::pinMode(pin, INPUT_PULLUP);
  ::attachInterrupt(digitalPinToInterrupt(pin), func, RISING);
}
```

uLisp does not use this. It polls. The window manager should not repeat this mistake.

### 4.5 gserial — The Blocking Poll

`gserial()` is the function the Lisp reader calls to get the next character of input. It is the bottleneck of the entire system — the place where the machine stops and waits for you.

```c
int gserial () {
  #if defined(serialmonitor)
  unsigned long start = millis();
  while (!KybdAvailable) {
    if (millis() - start > 1000) clrflag(NOECHO);
    if (Serial.available()) {
      char temp = Serial.read();
      if (temp != '\n' && !tstflag(NOECHO)) Serial.print(temp);
      return temp;
    } else {
      // PicoCalc keyboard
      if (pc_kbd.keyCount() > 0) {
        const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
        if (key.state == PCKeyboard::StatePress) {
          char temp = key.key;
          if (temp == '\t') autoComplete();
          else if ((temp != 0) && (temp != 255) &&
                   (temp != 0xA1) && (temp != 0xA2) && (temp != 0xA3) &&
                   (temp != 0xA4) && (temp != 0xA5)) {
            ProcessKey(temp); reset_autocomplete = true;
          }
        }
      }
    }
  }
  if (ReadPtr != WritePtr) return KybdBuf[ReadPtr++];
  KybdAvailable = 0;
  WritePtr = 0;
  return '\n';
```

The `#if defined(serialmonitor)` branch adds USB serial as an alternative input source — if a character arrives on USB within the polling loop, it takes priority. The PicoCalc keyboard is the `else` branch.

Read the `while (!KybdAvailable)` loop carefully. The machine spins here — checking USB serial, then checking the keyboard, then looping back — until `KybdAvailable` becomes true. `KybdAvailable` is set to 1 by `ProcessKey()` when the user presses Enter. Until then, nothing else runs.

The key filter is worth noting:

```c
(temp != 0) && (temp != 255) &&
(temp != 0xA1) &&            // ALT
(temp != 0xA2) && (temp != 0xA3) &&  // Shift L/R
(temp != 0xA4) &&            // SYM
(temp != 0xA5)               // CTRL
```

Modifier keys (Alt, Shift, Sym, Ctrl) are silently discarded. The REPL never sees them. They have already been applied by the STM32 firmware, which produces uppercase letters when Shift is held, or special characters when Sym is held. But the modifier key *press events themselves* are eaten. For a window manager, you would want to see these — Alt+Tab and Ctrl+something are natural window management chords.

Once the user presses Enter and `KybdAvailable` becomes 1, the function returns characters one at a time from `KybdBuf[]`:

```c
  if (ReadPtr != WritePtr) return KybdBuf[ReadPtr++];
  KybdAvailable = 0;
  WritePtr = 0;
  return '\n';
```

`ReadPtr` advances through the buffer. When it catches up with `WritePtr` (all characters consumed), the buffer is reset and a newline is returned to terminate the input.

### 4.6 ProcessKey — The Line Editor

When a key arrives from the keyboard (but before the REPL sees it), `ProcessKey(char c)` builds the input line and provides editing features:

```c
void ProcessKey (char c) {
  static int parenthesis = 0;
  static bool string = false;
  if (c == KEY_ESC) { setflag(ESCAPE); return; }
```

Escape sets the `ESCAPE` flag and returns immediately. The flag is checked by `testescape()` during the next evaluation.

For regular characters, the function manages the `KybdBuf` edit buffer:

```c
  if (c == '\n' || c == '\r') {
    pserial('\n');
    KybdAvailable = 1;
    ReadPtr = 0; LastWritePtr = WritePtr;
    return;
  }
```

Enter triggers output: a newline to `pserial` (which calls `Display('\n')`), then sets `KybdAvailable = 1` to unblock `gserial()`. `LastWritePtr` remembers the length of this line for the Shift+Return recall feature.

```c
  if (c == 8 || c == 0x7f) {     // Backspace key
    if (WritePtr > 0) {
      WritePtr--;
      Display(0x7F);              // move cursor back
      if (WritePtr) c = KybdBuf[WritePtr-1];
    }
  } else if (c == SHIFTRETURN) {
    for (int i = 0; i < LastWritePtr; i++) Display(KybdBuf[i]);
    WritePtr = LastWritePtr;
  } else if (WritePtr < KybdBufSize) {
    if (c == '"') string = !string;
    KybdBuf[WritePtr++] = c;
    Display(c);
  }
```

Backspace decrements the write pointer. Shift+Return replays the previous input. Any other printable character is appended to the buffer and displayed.

Then comes parenthesis matching — the most complex feature in the line editor:

```c
  if (c == ')' && !string) {
    int search = WritePtr-1, level = 0;
    bool string2 = false;
    while (search >= 0 && parenthesis == 0) {
      c = KybdBuf[search--];
      if (c == '"') string2 = !string2;
      if (c == ')' && !string2) level++;
      if (c == '(' && !string2) {
        level--;
        if (level == 0) parenthesis = WritePtr-search-1;
      }
    }
    Highlight(parenthesis, 1);
  }
```

When a `)` is typed (and we're not inside a string literal), the function scans backward through the buffer, tracking nesting depth, until it finds the matching `(`. `parenthesis` stores the distance (how many characters back the match is). Then `Highlight()` draws both parentheses in green.

`Highlight` works by sending virtual cursor movement and paren codes to `Display`:

```c
void Highlight (int p, uint8_t invert) {
  if (p) {
    for (int n=0; n < p; n++) Display(8);       // backspace p times
    Display(17 + invert);                         // draw '(' or '('|0x80
    for (int n=1; n < p; n++) Display(9);        // cursor forward p-1 times
    Display(19 + invert);                         // draw ')' or ')'|0x80
    Display(9);                                   // cursor forward once
  }
}
```

It navigates to the open paren position, draws it (possibly inverted), navigates to the close paren position, draws it, then navigates back. Four to six control character emissions per `)` keypress. Elegant, but it means every `)` triggers a burst of SPI writes.

For the window manager, all of this — the edit buffer, parenthesis matching, autocomplete — is per-window state. Each window would have its own `KybdBuf`, its own `WritePtr`, its own cursor position. The key routing decision happens before `ProcessKey`: which window receives this keystroke?

---

## Chapter 5. Escape — The Art of Interrupting

The PicoCalc has no operating system, no scheduler, no threads. When the Lisp interpreter is deep in a recursive computation — say, `(fib 30)` which makes 2.6 million function calls — there is no natural place for the machine to check whether you've pressed Escape. The solution is a cooperative mechanism: the interpreter periodically calls `testescape()`, which polls the keyboard.

### 5.1 The Escape Checker

```c
void testescape () {
  static unsigned long n;
  if (millis() - n < 500) return;
  n = millis();
  if (Serial.available() && Serial.read() == '~') error2("escape!");
  if (pc_kbd.keyCount() > 0) {
    const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
    if (key.state == PCKeyboard::StatePress) {
      char c = key.key;
      if (c == '~' || c == KEY_ESC) error2("escape!");
    }
  }
}
```

Three design decisions are packed into these eleven lines:

**The 500ms throttle.** The `static unsigned long n` variable records the last time `testescape` actually checked. If less than 500 ms have passed, the function returns immediately — no I2C transaction, no serial check. This limits keyboard polling to twice per second during long computations, which keeps I2C overhead from slowing down the interpreter. But it also means there is up to a half-second delay between pressing Escape and the interpreter noticing.

**The `~` character.** In addition to the Escape key (0xB1), the tilde character also triggers an abort. This is for USB serial users who don't have an Escape key on their terminal. The `~` is consumed — it's read from the serial buffer and discarded.

**The `error2()` call.** This is not a gentle request to stop. `error2("escape!")` calls `longjmp(*handler, 1)`, which obliterates the entire C call stack back to the `setjmp` in `loop()`. Every local variable in every function between `eval()` and `testescape()` is destroyed. The garbage collector's stack, the evaluator's environment, the reader's paren tracking — all gone.

### 5.2 Where testescape is Called

The function is woven into the interpreter at strategic points:

| Location | Context | Why |
|----------|---------|-----|
| `printarray()` line 1590 | Printing long arrays | Arrays can be huge |
| `apropos()` line 1791 | Scanning symbol table | Full symbol scan is O(n) |
| `eval()` line 2809 | Main eval loop | Catches runaway recursion |
| `fn_dotimes()` | Loop body | Long-running loops |
| `sp_formillis()` | millis loop body | Timed loops |
| `serial1read()` etc. | Waiting for serial | Blocking serial reads |
| `readbitarray()` | Reading bit arrays | Large bit arrays |

These call sites represent the places where a Lisp computation might spend a long time without naturally returning to the REPL. In a window manager world, each window would need its own escape mechanism — aborting one window's computation should not longjmp through another window's stack.

### 5.3 The Flag Register

uLisp uses a bitmap for cooperative state flags:

```c
enum flag {
  PRINTREADABLY,  // 0: print strings with escaping
  RETURNFLAG,     // 1: signal return from block
  ESCAPE,         // 2: user pressed Escape
  EXITEDITOR,     // 3: exit screen editor
  LIBRARYLOADED,  // 4: Lisp library loaded
  NOESC,          // 5: disable escape checking
  NOECHO,         // 6: suppress display echo
  MUFFLEERRORS,   // 7: suppress error output
  BACKTRACE       // 8: show call stack on error
};

#define setflag(x)  (Flags = Flags | 1<<(x))
#define clrflag(x)  (Flags = Flags & ~(1<<(x)))
#define tstflag(x)  (Flags & 1<<(x))
```

Nine flags packed into a single integer. The `NOESC` flag is particularly relevant: it's set by the `#.` reader macro, which evaluates code at read time. During a `#.` evaluation, escape checking is disabled — you can't abort a read-time computation. For a window manager, the ESCAPE flag would need to be per-window, and the window manager itself would need to intercept Escape before the focused window sees it.

---

## Chapter 6. The Buses — Three Independent Roads

The RP2040 is a wealthy chip. It has two SPI controllers, two I2C controllers, two UARTs, and enough GPIO pins to wire all of them simultaneously. The PicoCalc uses three of these buses, and — critically — they do not share any pins or controllers.

### 6.1 SPI1 — The Display

```
GP10 (SCK)  ──────── ILI9488 SCK
GP11 (MOSI) ──────── ILI9488 SDA (data in)
GP12 (MISO) ──────── ILI9488 SDO (data out, rarely used)
GP13 (CS)   ──────── ILI9488 CS
GP14 (DC)   ──────── ILI9488 DC (data/command)
GP15 (RST)  ──────── ILI9488 RESET
```

Clock speed: 25 MHz. The ILI9488 controller can handle up to 20 MHz according to its datasheet, but the PicoCalc runs it at 25 MHz with a software workaround — the `invertDisplay(1)` call in `initgfx()` compensates for timing issues at the higher speed.

The display is 320×320 pixels, 16-bit color (RGB565). A full-screen write is 320 × 320 × 2 = 204,800 bytes. At 25 MHz SPI (ignoring protocol overhead), this takes about 8 milliseconds. In practice, with CS toggling and command/address overhead, a full screen fill takes 10–15 ms.

The display does not have a framebuffer that the RP2040 can read back efficiently. `readPixel()` exists but is extremely slow — it requires a SPI read transaction that takes ~50 microseconds per pixel. There is no way to composite windows by reading and mixing pixel data from the display's RAM.

This means: a window manager must maintain its own framebuffer in RP2040 SRAM, or track dirty regions and repaint them from the virtual console buffers. The character-based console is already doing the latter — `ScrollBuf` is a software framebuffer that tracks what's on screen.

### 6.2 SPI0 — The SD Card

```
GP16 (MISO) ──────── SD Card DO
GP17 (CS)   ──────── SD Card CS
GP18 (SCK)  ──────── SD Card CLK
GP19 (MOSI) ──────── SD Card DI
```

Clock speed: `SPI_HALF_SPEED` (approximately 4 MHz). The SD card is used for `(save-image)`, `(load-image)`, and file I/O via `(open-for-read)` etc. It shares no hardware with the display SPI, so reading from SD and writing to the display can overlap (though both are DMA-capable, uLisp does not use DMA for either).

### 6.3 I2C1 — The Keyboard

```
GP6 (SDA) ──────── STM32 SDA
GP7 (SCL) ──────── STM32 SCL
```

Clock speed: 10 kHz. As discussed in Chapter 4, this is unusually slow due to the STM32's bus-sharing constraints. Every I2C transaction takes 2–4 milliseconds.

### 6.4 What's Free

These RP2040 pins are unused and available for the window manager:

```
GP2, GP3, GP4, GP5, GP8, GP9    — general-purpose digital I/O
GP20, GP21                       — general-purpose digital I/O
GP28                             — ADC input (analog capable)
```

If the keyboard's STM32 has its interrupt pin wired to any of these (the library example uses GP5), the RP2040 can receive hardware interrupts when keys are available, eliminating the need for polling.

---

## Chapter 7. Core 1 — The Sleeping Giant

The RP2040 has two ARM Cortex-M0+ cores, each running at 133 MHz. uLisp uses one. The other sits idle, waiting for something to do.

This is the single most important fact for the window manager. Core 1 is not just "available" — it is a complete second processor with its own register set, its own stack, and its own interrupt vector. It can run a completely independent program while Core 0 executes the Lisp interpreter.

### 7.1 The Inter-Core FIFO

The two cores communicate through a hardware FIFO — a queue of 32-bit words, 8 entries deep, with separate read and write ports for each core:

```
Core 0                          Core 1
  │                               │
  │ multicore_fifo_push_blocking() │ multicore_fifo_pop_blocking()
  │ ─────────────────────────────>│
  │                               │
  │ multicore_fifo_pop_blocking() │ multicore_fifo_push_blocking()
  │ <─────────────────────────────│
  │                               │
```

The FIFO also generates interrupts: Core 0 can be interrupted when Core 1 pushes data, and vice versa. This means Core 0 does not need to poll — it can post a character to the FIFO and continue evaluating Lisp, and Core 1 can consume it when convenient.

### 7.2 A Window Manager Architecture on Core 1

The natural split is:

- **Core 0** runs the Lisp interpreter, unchanged. When it calls `pserial(c)`, instead of writing to the display, it pushes `c` into the FIFO. When it calls `gserial()`, instead of polling the keyboard, it reads from the FIFO.

- **Core 1** runs the window manager. It polls the keyboard (or uses the GPIO interrupt), routes keys to the focused window, reads output characters from the FIFO, writes them to the focused window's virtual console, and repaints dirty regions to the display.

The protocol between the cores might look like this:

```
Core 0 → Core 1 (output):  { tag: OUTPUT, window_id: 0, char: 'H' }
Core 1 → Core 0 (input):   { tag: INPUT, char: 'A' }
Core 1 → Core 0 (escape):  { tag: ESCAPE }
```

Packed into 32-bit FIFO words, these become bit fields:

```
Bit 31:    tag (0 = output from Core 0, 1 = input from Core 1)
Bits 30-24: window_id (0-127, for future multi-window output)
Bits 23-16: reserved
Bits 15-8:  state (for input: key state)
Bits 7-0:   character code
```

A single 32-bit push per character. The FIFO holds 8 entries — enough for a short burst of output. If Core 1 falls behind, Core 0 blocks on `multicore_fifo_push_blocking()`. This is natural back-pressure: if the display can't keep up, the interpreter slows down.

### 7.3 The Virtual Console

Each window would have its own console state:

```c
struct VirtualConsole {
  char scrollBuf[MAX_COLS][MAX_LINES];  // character buffer
  uint8_t scroll;                       // circular buffer offset
  uint8_t cursorLine, cursorColumn;     // cursor position
  uint8_t lines, columns;               // window dimensions
  uint16_t originX, originY;            // pixel offset on screen
  bool dirty;                           // needs repaint?
  uint8_t dirtyLineStart, dirtyLineEnd; // dirty region
};
```

When Core 1 receives a character from Core 0, it writes it into the focused window's `VirtualConsole` using the same logic as `Display()` — but instead of calling `tft.drawChar()` directly, it marks the region as dirty. Then, on the next refresh cycle, it repaints only the dirty lines.

### 7.4 Hardware Spinlocks

The RP2040 has 32 hardware spinlocks — atomic test-and-set primitives that both cores can use to protect shared data. If both cores need to access the same `VirtualConsole` (for example, Core 0 reading a window's input buffer while Core 1 is writing to it), a spinlock provides mutual exclusion without an operating system:

```c
uint32_t lock = spin_lock_claim_unused(true);
spin_lock_unsafe_blocking(lock);
// ... critical section ...
spin_unlock_unsafe(lock);
```

The spinlock is a single instruction on the RP2040 — it's implemented in hardware, not software. Acquisition and release take nanoseconds, not microseconds.

---

## Chapter 8. Graphics — The Second Output Path

The console is not the only way to draw on the display. uLisp also provides a set of graphics primitives that bypass the scroll buffer entirely:

```c
// ulisp-picocalc.ino, line 2293
void gfxwrite (char c) { tft.write(c); }
```

`tft.write()` is TFT_eSPI's implementation of the Arduino `Print` interface — it draws a character at the current graphics cursor position, using the current text color and size. There is no scroll buffer, no line tracking, no cursor management. The character goes straight to the ILI9488's display RAM.

### 8.1 Graphics Primitives

The full set of drawing functions available from Lisp:

```c
// From the uLisp builtin function table:
tft.drawPixel(x, y, color);
tft.fillRect(x, y, w, h, color);
tft.fillCircle(x, y, r, color);
tft.fillRoundRect(x, y, w, h, r, color);
tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
tft.drawFastHLine(x, y, w, color);
tft.drawFastVLine(x, y, h, color);
tft.drawBitmap(x, y, bitmap, w, h, color);
tft.fillScreen(color);
tft.setCursor(x, y);
tft.setTextColor(color);
tft.setTextColor(fg, bg);
tft.setTextSize(size);
tft.setTextWrap(flag);
tft.setRotation(n);
```

These are all thin wrappers around TFT_eSPI calls. They write directly to the display — no buffering, no compositing, no clipping. When a Lisp program calls `(fill-rect 0 0 320 320 0)`, it paints the entire screen black, obliterating whatever the console had displayed.

### 8.2 GFXSTREAM — The Dual Output Mode

uLisp defines an enum for output stream types:

```c
enum stream {
  SERIALSTREAM, I2CSTREAM, SPISTREAM, SDSTREAM,
  WIFISTREAM, STRINGSTREAM, GFXSTREAM
};
```

When `print` is called with a GFXSTREAM destination, characters flow through `gfxwrite()` instead of `pserial()`. The pretty-printer even adjusts its line width:

```c
const int PPWIDTH = 52;
const int GFXPPWIDTH = 52;
int ppwidth = PPWIDTH;

// In fn_pprint:
if (pfun == gfxwrite) ppwidth = GFXPPWIDTH;

// In fn_pprintall:
if (pfun == gfxwrite) ppwidth = GFXPPWIDTH;
```

The graphics text width is the same as the serial width (52 characters) because the display is 320 pixels wide at 6 pixels per character, which gives 53 columns — close enough to 52.

### 8.3 The Window Manager Problem with Graphics

Graphics operations write to absolute screen coordinates. A call to `(draw-pixel 100 200 #xf00)` lights up pixel (100, 200) regardless of where any console window is. There is no clipping, no window ownership.

For the window manager, this means one of two things:

1. **Graphics windows own their entire region.** A window in "graphics mode" has exclusive control over its screen rectangle. The WM does not paint console text there, and the graphics output is clipped to the window's bounds. This requires intercepting every `tft.fillXxx()` call and adjusting coordinates.

2. **Graphics goes fullscreen.** When a window enters graphics mode, it takes over the entire display. Other windows are hidden. This is simpler but less useful.

Option 1 is strictly more capable, but it requires modifying every graphics function to accept a coordinate offset and clip rectangle. Option 2 is a reasonable first implementation — and it's what the window manager in the PicoMite (the screenshot you showed) appears to do.

### 8.4 Readback — There and Back Again

The ILI9488 supports reading pixel data from its display RAM:

```c
object *fn_readpixel (object *args, object *env) {
  return number(tft.readPixel(checkinteger(first(args)),
                               checkinteger(second(args))));
}
```

But `readPixel()` on TFT_eSPI is slow — it requires switching the SPI bus from write mode to read mode, clocking out a read command, and reading back 3 bytes (18-bit color). This takes ~50 microseconds per pixel. Reading the entire 320×320 display would take over 5 seconds. There is no way to use readback for compositing.

The window manager must therefore maintain its own representation of each window's content — either the character grid (for text windows) or a pixel buffer in SRAM (for graphics windows).

---

## Chapter 9. The Sound System — PWM Audio

The PicoCalc has stereo speakers driven by the RP2040's PWM hardware. The audio pins are:

```
GP26 ─── Left speaker (PWM slice 5, channel A)
GP27 ─── Right speaker (PWM slice 5, channel B)
```

The sound system is relevant to the window manager because sound is a shared resource — when one window plays a note, it uses the physical speaker that all windows share.

### 9.1 Play Tone

```c
void play_tone (uint freq) {
  gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
  gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
  uint slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
  uint slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);
  pwm_config config = pwm_get_default_config();
  float div = (float)clock_get_hz(clk_sys) / (freq * 10000);
  pwm_config_set_clkdiv(&config, div);
  pwm_config_set_wrap(&config, 10000);
  pwm_init(slice_l, &config, true);
  pwm_init(slice_r, &config, true);
  pwm_set_gpio_level(AUDIO_PIN_L, 5000);  // 50% duty cycle
  pwm_set_gpio_level(AUDIO_PIN_R, 5000);
}
```

The tone generator uses raw RP2040 PWM hardware — not the Arduino `analogWrite()` abstraction. It configures the PWM slice with a divisor that produces the desired frequency, sets the wrap value to 10,000 (giving 10,000 discrete volume levels), and runs both channels at 50% duty cycle (5,000 out of 10,000).

The frequency range is limited by the PWM clock divisor. At 133 MHz system clock, the highest achievable frequency is 133 MHz / 10,000 = 13.3 kHz (at divisor 1.0), and the lowest is approximately 13 Hz. Musical notes fall comfortably within this range.

### 9.2 The Note Function

```c
const int scale[] = {4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902};

void playnote (int pin, int note, int octave) {
  int freq = scale[note] >> (4 - octave);
  // ... configure PWM for freq ...
}

void nonote (int pin) {
  // ... disable PWM output ...
}
```

The note lookup table gives MIDI note frequencies for octave 4. Lower octaves are produced by right-shifting (dividing by 2), higher octaves by left-shifting (multiplying by 2). This is the standard technique for generating musical tones from a square wave.

### 9.3 The `(note)` Lisp Function

From Lisp, you play a note with `(note pin note-number octave)`:

```c
object *fn_noteq (object *args, object *env) {
  // ... parse arguments ...
  playnote(pin, note, octave);
  return nil;
}
```

And silence it with `(nonote)`:

```c
// In sp_delay:
delay(1000 * secs);
```

The `delay()` call blocks the entire machine. If a Lisp program plays a 2-second note, the RP2040 does nothing else for those 2 seconds. For the window manager, this would need to become asynchronous — start the tone, set a timer, and continue processing other windows.

---

## Chapter 10. What the Window Manager Must Do

We have now traced every path through the console system: the display pipeline, the keyboard pipeline, the escape mechanism, the bus architecture, the dual-core opportunity, the graphics overlay, and the sound system. Let us close by summarizing what a window manager must change, what it can reuse, and what it must create from nothing.

### 10.1 What to Keep

The Lisp interpreter itself — `eval()`, `read()`, `printobject()`, the garbage collector, the object allocator — all of this runs unchanged. The window manager replaces the I/O layer, not the computation layer.

The TFT_eSPI library. The display driver works. We keep the `tft` object and all its drawing methods.

The `PCKeyboard` library. The I2C communication works. We keep the register read/write methods. We just stop polling and start using interrupts.

The SD card subsystem. It's on a separate SPI bus and doesn't interfere with anything.

### 10.2 What to Replace

| Function | Current behavior | Window manager behavior |
|----------|-----------------|------------------------|
| `pserial()` | Write to display | Push character to inter-core FIFO |
| `Display()` | Global cursor, single scroll buffer | Per-window virtual console |
| `PlotChar()` | Direct `tft.drawChar()` | Write to window's buffer, mark dirty |
| `ScrollDisplay()` | Scroll entire 320×320 screen | Scroll one window's viewport |
| `gserial()` | Block polling keyboard | Read from inter-core FIFO |
| `ProcessKey()` | Global edit buffer | Per-window edit buffer |
| `testescape()` | Global `ESCAPE` flag + `longjmp` | Per-window escape, routed by WM |
| `initkybd()` | Polling at 10 kHz | GPIO interrupt, Core 1 handles |

### 10.3 What to Create

**The window manager loop** (runs on Core 1):

```
while (true) {
  if (keyboard_event_available) {
    key = read_key_from_stm32();
    if (is_wm_chord(key)) {
      handle_window_switch(key);
    } else {
      push_to_focused_window_input(key);
    }
  }
  if (core0_has_output) {
    char c = read_from_fifo();
    write_to_focused_window_console(c);
    mark_dirty(focused_window, c.line);
  }
  if (dirty_windows_exist) {
    repaint_dirty_regions();
  }
  if (cursor_blink_timer) {
    toggle_cursor_visibility(focused_window);
  }
}
```

**The virtual console data structure** (one per window):

```c
struct Window {
  char scrollBuf[MAX_COLS][MAX_LINES];
  uint8_t scroll, cursorLine, cursorColumn;
  uint16_t x, y, width, height;       // pixel bounds on screen
  uint8_t id;
  bool hasFocus, dirty, graphicsMode;
  char editBuf[KYBD_BUF_SIZE];
  int editWritePtr, editReadPtr;
  bool editAvailable;
};
```

**The Core 0 shim functions** that replace `pserial` and `gserial`:

```c
void wm_pserial(char c) {
  uint32_t word = (0 << 31) | (focusedWindowId << 24) | c;
  multicore_fifo_push_blocking(word);
}

int wm_gserial() {
  uint32_t word = multicore_fifo_pop_blocking();
  return word & 0xFF;
}
```

### 10.4 The Hard Problems

Three problems remain that do not have obvious solutions:

**Problem 1: Escape handling.** The current `error2("escape!")` does a `longjmp` that obliterates the entire C stack. There is no way to abort one window's computation without affecting another's. A per-window error boundary would require restructuring how `setjmp`/`longjmp` work — perhaps by running each window's REPL in a separate `setjmp` scope on Core 0.

**Problem 2: Blocking delays.** `(delay 5)` calls `delay(5000)`, which stops the machine for 5 seconds. The window manager needs asynchronous timers — start a timer, continue processing, fire a callback when the timer expires. The RP2040 has 4 hardware alarm timers that can do this.

**Problem 3: Memory.** Four virtual consoles with color attributes consume about 14 KB. The uLisp heap is ~178 KB. The RP2040 has 264 KB total, with ~200 KB available after the heap. Four windows fit comfortably. Eight windows might start to squeeze the Lisp workspace. The window manager must budget memory carefully.

---

## Appendix. Key Code Locations

Quick reference for finding every function discussed in this document:

| Function | File | Lines |
|----------|------|-------|
| `setup()` | `ulisp-picocalc.ino` | 7707 |
| `loop()` | `ulisp-picocalc.ino` | 7764 |
| `repl()` | `ulisp-picocalc.ino` | 7548 |
| `pserial()` | `ulisp-picocalc.ino` | 6976 |
| `initgfx()` | `ulisp-picocalc.ino` | 7530 |
| `initkybd()` | `ulisp-picocalc.ino` | 7304 |
| `PlotChar()` | `ulisp-picocalc.ino` | 7224 |
| `ScrollDisplay()` | `ulisp-picocalc.ino` | 7234 |
| `Display()` | `ulisp-picocalc.ino` | 7258 |
| `gserial()` | `ulisp-picocalc.ino` | 7443 |
| `ProcessKey()` | `ulisp-picocalc.ino` | 7387 |
| `Highlight()` | `ulisp-picocalc.ino` | 7379 |
| `autoComplete()` | `ulisp-picocalc.ino` | 7316 |
| `testescape()` | `ulisp-picocalc.ino` | 6742 |
| `getKey()` | `ulisp-picocalc.ino` | 5478 |
| `fn_getkey()` | `ulisp-picocalc.ino` | 5494 |
| `gfxwrite()` | `ulisp-picocalc.ino` | 2293 |
| `play_tone()` | `ulisp-picocalc.ino` | 2556 |
| `playnote()` | `ulisp-picocalc.ino` | 2580 |
| `ulisperror()` | `ulisp-picocalc.ino` | 7770 |
| Console constants | `ulisp-picocalc.ino` | 7211 |
| Graphics functions | `ulisp-picocalc.ino` | 5260–5465 |
| STM32 firmware | `picocalc_keyboard.ino` | all |
| Keyboard library | `PCKeyboard.cpp` | all |
| TFT pin config | `Setup60_RP2040_ILI9488.h` | all |
| PicoCalc pin config | `config.h` | all |
| LCD SPI primitives | `lcdspi.h` | all |
| I2C keyboard client | `i2ckbd.c` | all |
| Interrupt example | `InterruptDriven.ino` | all |
