# uLisp PicoCalc Source Index

> Navigation map for `ulisp-picocalc/ulisp-picocalc.ino` (7,793 lines, uLisp 4.8f).
> Use line numbers to jump directly in CLion (**Ctrl+G** → enter line number).

---

## Welcome to the Project

You are looking at a single 7,793-line C++ file that contains an entire Lisp
interpreter — reader, evaluator, printer, garbage collector, editor, ARM
assembler, graphics engine, terminal emulator, and filesystem — compiled into a
460-kilobyte binary that runs on a $4 microcontroller inside a handheld computer
you can hold in one hand.

### The Hardware

The **Clockwork Pi PicoCalc** is a handheld computer built around a **Raspberry
Pi Pico** — a tiny circuit board containing an **RP2040** microcontroller with
264 KB of RAM and 2 MB of flash storage. It has:

- A **320×320 color IPS display** (ILI9488 controller, driven via SPI)
- A **QWERTY keyboard** (scanned by a separate STM32 microcontroller, communicated over I2C)
- An **SD card slot** (connected via SPI)
- A **speaker** (driven by PWM on GPIO pins 26 and 27)
- **USB-C** for power, charging, and serial communication

### The Software

**uLisp** is a subset of Common Lisp designed for microcontrollers, created by
David Johnson-Davies ([ulisp.com](https://ulisp.com)). The version in this file
is **uLisp 4.8f**, customized for the PicoCalc. When you flash this firmware
onto the PicoCalc, it boots into a Lisp REPL — a screen that shows a `>` prompt
and waits for you to type a Lisp expression. You type `(fact 10)`, press Enter,
and it prints `3628800`.

### How It All Fits Together

When the PicoCalc powers on:

1. **`setup()`** (line 7706) runs once — initializes the display, keyboard,
   memory, and prints "uLisp 4.8f"
2. **`loop()`** (line 7766) calls **`repl()`** (line 7724), which is an infinite loop:
   - **Read**: `readmain(gserial)` reads characters from the keyboard and builds Lisp objects
   - **Eval**: `eval(line, env)` evaluates the expression
   - **Print**: `printobject(result, pserial)` displays the result on screen
3. If anything goes wrong, `longjmp` jumps to `ulisperror()` (line 7779), which cleans up and restarts the REPL

The data flows like this:

```
Keyboard (I2C) → gserial() → readmain() → Lisp objects → eval() → result → printobject() → pserial() → Display() → TFT screen
```

Everything else in the file — the garbage collector, the symbol table, the 500+
built-in functions, the ARM assembler — exists to support this loop.

### This File Is the Whole Program

There is no build system beyond `arduino-cli compile`. There are no other source
files, no headers to include (beyond the Arduino libraries). This single `.ino`
file is the entire firmware. The sections below walk you through it from top to
bottom. Each section explains what the code does, why it exists, and how it
connects to the rest of the system.

---

## Table of Contents

- [1. Compile Options & Includes](#1-compile-options--includes)
- [2. Type System & Macros](#2-type-system--macros)
- [3. Global State & Workspace](#3-global-state--workspace)
- [4. Error Handling](#4-error-handling)
- [5. Memory Management & GC](#5-memory-management--gc)
- [6. Object Constructors](#6-object-constructors)
- [7. Symbol Interning](#7-symbol-interning)
- [8. Image Save/Load (Flash & SD)](#8-image-saveload-flash--sd)
- [9. Tracing](#9-tracing)
- [10. Predicates & Type Checking](#10-predicates--type-checking)
- [11. Radix-40 Encoding](#11-radix-40-encoding)
- [12. Equality & Comparison](#12-equality--comparison)
- [13. Arithmetic Internals](#13-arithmetic-internals)
- [14. Arrays](#14-arrays)
- [15. Strings](#15-strings)
- [16. Environment & Closures](#16-environment--closures)
- [17. I/O Streams & Serial](#17-io-streams--serial)
- [18. Pretty Printing](#18-pretty-printing)
- [19. Editor & ARM Assembler](#19-editor--arm-assembler)
- [20. Special Forms (sp_)](#20-special-forms-sp_)
- [21. Tail-Call Forms (tf_)](#21-tail-call-forms-tf_)
- [22. Built-in Functions (fn_)](#22-built-in-functions-fn_)
- [23. Data Tables (Symbol Names, Docs, Lookup)](#23-data-tables-symbol-names-docs-lookup)
- [24. Evaluator](#24-evaluator)
- [25. Printer (Output)](#25-printer-output)
- [26. Display & Keyboard (PicoCalc-specific)](#26-display--keyboard-picocalc-specific)
- [27. Reader (Input/Parser)](#27-reader-inputparser)
- [28. Setup, REPL & Entry Points](#28-setup-repl--entry-points)

---

## 1. Compile Options & Includes

The first 61 lines are configuration — they tell the compiler what features to include and what hardware to target. These are not runtime settings; changing one requires a recompile.

The `#define` feature flags control what gets compiled in: `printfreespace` shows free memory before each prompt, `sdcardsupport` enables SD card operations, `gfxsupport` turns on the PicoCalc display and keyboard, and `assemblerlist` includes the ARM machine code assembler. Commenting out a feature removes it from the binary, saving flash and RAM.

Below the flags, the hardware includes wire up the PicoCalc-specific peripherals: **LittleFS** for a persistent filesystem in flash, **TFT_eSPI** for the display driver, **PCKeyboard** for the I2C keyboard, and **hardware/pwm.h** for speaker output. The line `#define Serial Serial1` is notable — it redirects the default `Serial` to the hardware UART instead of USB, because the PicoCalc's keyboard uses the UART pins.

| Lines | Description |
|------:|-------------|
| 1–6 | Header comment (version, license) |
| 8 | `LispLibrary[]` — empty (no preloaded library) |
| 10–21 | Compile options: `printfreespace`, `serialmonitor`, `sdcardsupport`, `gfxsupport`, `assemblerlist` |
| 22–30 | `#include`: `<setjmp.h>`, `<SPI.h>`, `<Wire.h>`, `<limits.h>` |
| 32–34 | SD card settings: `SDCARD_SS_PIN=17`, `SDSIZE=720` |
| 38–42 | LittleFS config: `#define LITTLEFS`, file mode strings |
| 44–55 | PicoCalc hardware: `TFT_eSPI`, `PCKeyboard`, `AUDIO_PIN_L/R` (26,27), `Serial=Serial1`, `KEY_ESC=0xB1` |
| 56–61 | Platform settings: `WORDALIGNED`, `BUFFERSIZE=36`, `RAMFUNC`, `MEMBANK` |

### Key Constants

| Line | Constant | Value |
|-----:|----------|-------|
| 53 | `AUDIO_PIN_L` | 26 |
| 54 | `AUDIO_PIN_R` | 27 |
| 50 | `KEY_ESC` | 0xB1 |
| 32 | `SDCARD_SS_PIN` | 17 |
| 59 | `BUFFERSIZE` | 36 |

---

## 2. Type System & Macros

This section defines the fundamental vocabulary of the interpreter. The most important definition is the **`object` struct** (line 197) — an 8-byte tagged union that represents every Lisp value. When `type` is `PAIR` or `STRING`, the struct is interpreted as two pointers (`car` and `cdr`). When `type` is `NUMBER`, `FLOAT`, `SYMBOL`, or `CHARACTER`, it holds a type tag plus a value. Everything in uLisp — numbers, strings, symbols, lists, functions — is one of these 8-byte objects allocated from a flat array called `Workspace`.

Below the struct, a dense block of C macros (lines 132–183) provides the daily vocabulary of the codebase: `car(x)`/`cdr(x)` for list access, `push(x,y)`/`pop(y)` for stack operations, `protect(y)`/`unprotect()` for GC roots, `integerp(x)`/`floatp(x)` for type checks, and `mark(x)`/`unmark(x)` for garbage collection. You will see these macros used in nearly every function.

The board selection block (lines 63–130) uses `#if defined(...)` to set `WORKSPACESIZE` — the number of objects the Lisp heap can hold. On the base Pico it's 22,280 objects (178 KB); on the Pico 2 it's 46,280 (370 KB). The `- SDSIZE` subtracts space for SD card buffers when `sdcardsupport` is enabled.

| Lines | Description |
|------:|-------------|
| 63–80 | `WORKSPACESIZE`, `BACKTRACESIZE`, `TRACEMAX` — memory sizing |
| 82–131 | `struct sobject` — the 8-byte Lisp object (type, car, cdr union) |
| 132–171 | Core macros: `car`, `cdr`, `first`–`third`, `push`/`pop`, `protect`/`unprotect`, type predicates (`integerp`, `floatp`, etc.), GC marks (`mark`/`unmark`/`marked`), flags (`setflag`/`clrflag`/`tstflag`), `twist`/`untwist` (symbol name encoding) |
| 187 | `enum type` — object types: `ZZERO`, `SYMBOL`, `CODE`, `NUMBER`, `STREAM`, `CHARACTER`, `FLOAT`, `ARRAY`, `STRING`, `PAIR` |
| 188 | `enum token` — parser tokens: `BRA`, `KET`, `QUO`, `DOT` |
| 189 | `enum fntypes_t` — function categories: `OTHER_FORMS`, `TAIL_FORMS`, `FUNCTIONS`, `SPECIAL_FORMS` |
| 193–195 | `typedef symbol_t`, `builtin_t`, `chars_t` (all `uint32_t`) |
| 215 | `fn_ptr_type` — function pointer signature for builtins |
| 216 | `mapfun_t` — map function callback |
| 219–234 | `tbl_entry_t`, `nstream_t`, `gfun_t`/`pfun_t`, stream table types |
| 242 | `stream_entry_t` — I/O stream dispatch table entry |
| 278 | `enum flag` — runtime flags: `PRINTREADABLY`, `RETURNFLAG`, `ESCAPE`, etc. |

---

## 3. Global State & Workspace

A handful of global variables (lines 250–283) track the interpreter's entire runtime state. The most important:

- **`Workspace[WORKSPACESIZE]`** — The Lisp heap: a flat array of `object` structs. On the Pico, about 178 KB.
- **`GlobalEnv`** — The global environment: an association list of `(symbol . value)` pairs. When you type `(defvar x 42)`, it adds `(x . 42)` here.
- **`GCStack`** — A stack of objects the garbage collector must not free. Functions `protect()` and `unprotect()` manage it around allocations.
- **`Freelist`** — A linked list of free objects. `myalloc()` pops from it; `myfree()` pushes to it.
- **`handler`** — A `jmp_buf*` for error recovery via `longjmp`. When an error occurs anywhere in `eval()`, execution teleports back to `loop()`.
- **`Flags`** — A bitmask of booleans: `PRINTREADABLY`, `RETURNFLAG` (user abort), `ESCAPE` (Escape pressed), `BACKTRACE` (show call trace).

| Line | Variable | Description |
|-----:|----------|-------------|
| 250 | `Workspace[WORKSPACESIZE]` | The Lisp heap (in MEMBANK) |
| 255 | `toplevel_handler` | `jmp_buf` for top-level error recovery |
| 256 | `*handler` | Current error handler (points into closure stack) |
| 257 | `Freespace` | Free object count |
| 258 | `*Freelist` | Head of free list |
| 259 | `I2Ccount` | I2C byte counter |
| 260 | `TraceFn[]` | Traced function names |
| 261 | `TraceDepth[]` | Trace nesting depth |
| 264 | `TraceStart`, `TraceTop` | Circular trace buffer indices |
| 267 | `*GlobalEnv` | Global environment (alist) |
| 268 | `*GCStack` | GC root stack |
| 269 | `*GlobalString` | Current output string |
| 270 | `*GlobalStringTail` | Tail pointer for string building |
| 272 | `PrintCount` | Pretty-print column counter |
| 273 | `BreakLevel` | Debugger break depth |
| 283 | `*tee` | The `t` symbol object |

---

## 4. Error Handling

uLisp uses C's `setjmp`/`longjmp` for error recovery — a `goto` across function calls. When something goes wrong deep inside `eval()`, the code calls `longjmp` to teleport back to `loop()`, which calls `ulisperror()` to clean up and restart the REPL.

The flow works like this: `loop()` calls `setjmp(toplevel_handler)` to set a landing point, then enters `repl()`. If `eval()` encounters an error (wrong argument type, division by zero, undefined variable), it calls `error("message", symbol)`, which formats the message, prints it, and calls `errorend()` — a `longjmp` that unwinds the entire C stack back to `loop()`.

The `handler` pointer can point into nested `setjmp` targets for debugger breakpoints, but at the top level it always points to `toplevel_handler`.

| Line | Function | Description |
|-----:|----------|-------------|
| 288 | `modbacktrace(n)` | Circular buffer index modulo |
| 292 | `printbacktrace()` | Print `fn ← fn ← fn` chain |
| 302 | `errorsub(fname, string)` | Print "Error: 'fname': string" |
| 315 | `errorend()` | `longjmp` to error handler |
| 317 | `errorsym(fname, string, symbol)` | Error with symbol context |
| 327 | `errorsym2(fname, string)` | Error without symbol |
| 335 | `error(string, symbol)` | Generic error |
| 339 | `error2(string)` | Error without symbol |
| 343 | `formaterr(formatstr, string, p)` | Format string error at position |
| 352–354 | Error strings | `"argument is not a number"`, etc. |

---

## 5. Memory Management & GC

Memory allocation is dead simple: `initworkspace()` chains all objects in the workspace into a singly-linked free list. `myalloc()` pops the first free object; `myfree()` pushes one back. There is no malloc, no heap, no fragmentation — just a flat array and a linked list.

When free space drops below 1/16 of the workspace, the mark-and-sweep garbage collector runs. **`markobject()`** starts from the roots (`GlobalEnv`, `GCStack`, the current form and environment) and recursively walks every reachable object, setting a MARKBIT in the low bit of its `car` pointer. Then **`sweep()`** walks the entire workspace array: unmarked objects go to the free list, marked objects get their bit cleared for the next cycle.

The mark function uses `goto MARK` instead of recursive calls for the `cdr` traversal — this avoids C stack overflow when marking long lists, which matters on a microcontroller with limited stack space.

`compactimage()` (line 627) is used by `save-image` to slide all live objects into a contiguous block, adjusting every internal pointer to match the new locations.

| Line | Function | Description |
|-----:|----------|-------------|
| 378 | `initworkspace()` | Initialize free list |
| 389 | `myalloc()` | Allocate one object from free list |
| 397 | `myfree(obj)` | Return object to free list |
| 544 | `markobject(obj)` | Mark-and-sweep: mark phase |
| 574 | `sweep()` | Mark-and-sweep: sweep phase |
| 583 | `gc(form, env)` | Run garbage collector, print free space |
| 600 | `movepointer(from, to)` | Compact image: relocate pointer |
| 627 | `compactimage(arg)` | Compact workspace for save-image |

---

## 6. Object Constructors

These functions create new Lisp objects by popping a free slot from the free list and filling in its fields. `number(n)` sets `type = NUMBER, integer = n`. `cons(arg1, arg2)` sets `car = arg1, cdr = arg2` — this is the list builder; `(list 1 2 3)` is three nested cons cells. `symbol(name)` creates a symbol with a packed radix-40 name. `stream(type, address)` wraps an I/O channel (serial, I2C, SPI, SD card) as a first-class Lisp object.

Every allocation should be wrapped in `protect()`/`unprotect()` calls so the GC doesn't reclaim the object during a subsequent allocation inside the same function.

| Line | Function | Description |
|-----:|----------|-------------|
| 406 | `number(n)` | Create integer object |
| 413 | `makefloat(f)` | Create float object |
| 420 | `character(c)` | Create character object |
| 427 | `cons(arg1, arg2)` | Create pair (cons cell) |
| 434 | `symbol(name)` | Create symbol object |
| 441 | `bsymbol(name)` | Create builtin symbol |
| 445 | `codehead(entry)` | Create code header |
| 498 | `newstring()` | Create empty string |
| 518 | `features()` | Build `*features*` list |

---

## 7. Symbol Interning

Lisp symbols must be unique — there should be only one object representing the symbol `foo`. **`intern(name)`** (line 452) scans the workspace for an existing symbol with that name; if found, it returns the existing object. If not, it creates a new one. This ensures that `eq` comparison on symbols is just a pointer compare.

Symbol names up to 5 characters are packed into a single 32-bit word using radix-40 encoding (40 characters: a–z, 0–9, hyphen). Longer names are stored as a linked list of packed words, with the low two bits of the name field being 0 to signal "this is a long name" — see `internlong()` (line 479).

| Line | Function | Description |
|-----:|----------|-------------|
| 452 | `intern(name)` | Intern a radix-40 symbol name |
| 462 | `eqsymbols(obj, buffer)` | Compare long symbol names |
| 479 | `internlong(buffer)` | Intern a long symbol name (>5 chars) |
| 491 | `stream(type, address)` | Create stream object |

---

## 8. Image Save/Load (Flash & SD)

uLisp can save and restore the entire Lisp workspace — all your variables, function definitions, and data — to persistent storage. Power off, power back on, and everything is still there.

There are two backends: **flash memory** (the RP2040's 2 MB flash, using the upper portion above the firmware) and **SD card** (when `sdcardsupport` is enabled). The format is simple: `saveimage()` (line 902) writes the workspace array and global environment as raw bytes; `loadimage()` (line 985) reads them back. `autorunimage()` (line 1066) loads a saved image automatically on boot.

The Flash helper functions (lines 712–897) handle the low-level details of writing to the RP2040's flash: enabling writes, erasing 4 KB sectors, writing 256-byte pages, and reading back. There are three conditional blocks of these functions for different storage backends (raw flash, LittleFS, and SD).

### SD Card Helpers
| Line | Function | Description |
|-----:|----------|-------------|
| 653 | `MakeFilename(arg, buffer)` | Convert Lisp string to C filename |
| 670 | `SDBegin()` | Initialize SD card |
| 674 | `SDWrite32(file, data)` | Write 32-bit to SD |
| 679 | `SDRead32(file)` | Read 32-bit from SD |
| 685 | `FSWrite32(file, data)` | Write 32-bit to LittleFS |
| 691 | `FSRead32(file)` | Read 32-bit from LittleFS |

### Flash Helpers
| Line | Function | Description |
|-----:|----------|-------------|
| 712 | `FlashBusy()` | Wait for flash ready |
| 719 | `FlashWrite(data)` | Write byte to flash |
| 723 | `FlashReadByte()` | Read byte from flash |
| 727 | `FlashWriteByte(addr, data)` | Write byte at address |
| 743 | `FlashWriteEnable()` | Enable flash writes |
| 749 | `FlashCheck()` | Verify flash chip ID |
| 764 | `FlashBeginWrite(addr, bytes)` | Start flash write sequence |
| 778–804 | `FlashWrite32` / `FlashEndWrite` / `FlashBeginRead` / `FlashRead32` / `FlashEndRead` | Flash I/O primitives |
| 813–865 | `row_erase` / `page_clear` / `page_write` + conditional Flash stubs | Flash erase/write internals |

### Image Operations
| Line | Function | Description |
|-----:|----------|-------------|
| 902 | `saveimage(arg)` | Save workspace to flash/SD |
| 985 | `loadimage(arg)` | Load workspace from flash/SD |
| 1066 | `autorunimage()` | Auto-load saved image on boot |

---

## 9. Tracing

uLisp has a built-in tracing facility: `(trace foo)` causes every call to `foo` to print its arguments and return value, indented by call depth. Up to 3 functions can be traced simultaneously (`TRACEMAX = 3`). The trace is implemented inside `eval()` — when it detects that the function being called is traced, it wraps the call in a recursive `eval()` (instead of the usual `goto EVAL` tail call) so it can capture and print the return value.

| Line | Function | Description |
|-----:|----------|-------------|
| 1104 | `tracing(name)` | Check if function is traced |
| 1113 | `trace(name)` | Add function to trace list |
| 1123 | `untrace(name)` | Remove function from trace list |

---

## 10. Predicates & Type Checking

These are the guard functions that validate arguments before operating on them. Every built-in function starts with calls like `checkinteger(first(args))` — if the caller passed the wrong type, these functions call `error()` which longjmps back to the REPL. `consp(x)` and `listp(x)` are the fundamental type predicates; the difference is that `listp` returns true for `nil` (the empty list) while `consp` does not.

| Line | Function | Description |
|-----:|----------|-------------|
| 1134 | `consp(x)` | Is it a pair? |
| 1142 | `listp(x)` | Is it nil or a pair? |
| 1156 | `builtin(name)` | Symbol → builtin index |
| 1160 | `sym(x)` | Builtin index → symbol |
| 1206 | `checkinteger(obj)` | Extract integer or error |
| 1211 | `checkbitvalue(obj)` | Extract bit value |
| 1218 | `checkintfloat(obj)` | Extract integer-as-float |
| 1224 | `checkchar(obj)` | Extract character |
| 1229 | `checkstring(obj)` | Extract string |
| 1234 | `isstream(obj)` | Check stream type |
| 1239 | `isbuiltin(obj, n)` | Check builtin equality |
| 1243 | `builtinp(name)` | Is it a builtin symbol? |
| 1247 | `checkkeyword(obj)` | Check keyword |
| 1255 | `checkargs(args)` | Validate argument count |

---

## 11. Radix-40 Encoding

Symbols are compressed using a base-40 alphabet (a–z, 0–9, hyphen = 40 characters). Five such characters fit in a 32-bit word (40⁵ = 102,400,000 < 2³²). Short symbol names (≤5 chars) are stored inline in the symbol object's `name` field; longer names are stored as a linked list of packed words. The functions `toradix40`/`fromradix40` convert between characters and values; `pack40` packs 5 chars into a word; `twist`/`untwist` rearrange bits so the low two bits encode the name length.

Symbols are packed into 32-bit words using a 40-character alphabet (letters + digits + hyphen). This is the core symbol compression scheme.

| Line | Function | Description |
|-----:|----------|-------------|
| 1164 | `toradix40(ch)` | Character → radix-40 value |
| 1173 | `fromradix40(n)` | Radix-40 value → character |
| 1180 | `pack40(buffer)` | Pack 5 chars into 32-bit symbol |
| 1189 | `valid40(buffer)` | Validate radix-40 symbol |
| 1199 | `digitvalue(d)` | Hex digit → integer |

---

## 12. Equality & Comparison

uLisp implements two equality predicates matching Common Lisp: **`eq`** (line 1276) is pointer equality — two objects are `eq` if they are the same memory address (numbers and characters are compared by value since uLisp interns them). **`equal`** (line 1292) is structural equality — it recursively walks lists, comparing elements, so `(equal '(1 2 3) '(1 2 3))` is true even though they are different cons cells.

The `compare()` function (line 1388) is the shared engine behind all six numeric comparisons (`<`, `<=`, `>`, `>=`, `=`, `/=`), parameterized by three boolean flags.

| Line | Function | Description |
|-----:|----------|-------------|
| 1260 | `eqlongsymbol(s1, s2)` | Compare long symbol names |
| 1270 | `eqsymbol(s1, s2)` | Compare symbol names |
| 1276 | `eq(arg1, arg2)` | Pointer/integer/char equality |
| 1292 | `equal(arg1, arg2)` | Structural equality (recursive) |
| 1298 | `listlength(list)` | Length of proper list |
| 1308 | `checkarguments(args, min, max)` | Validate arg range |
| 1388 | `compare(args, lt, gt, eq)` | Numeric comparison engine |

---

## 13. Arithmetic Internals

uLisp supports both integers and floats with automatic promotion — if you add an integer and a float, the result is a float. The `add_floats`/`subtract_floats`/`multiply_floats`/`divide_floats` functions handle the mixed-type arithmetic by accumulating into a `float` result when any operand is a float. `remmod(args, mod)` implements both `rem` and `mod` (they differ in sign handling for negative numbers). `intpower(base, exp)` is the integer exponentiation used by the radix-40 encoder.

| Line | Function | Description |
|-----:|----------|-------------|
| 1320 | `add_floats(args, fresult)` | Sum with float promotion |
| 1329 | `subtract_floats(args, fresult)` | Subtraction with float promotion |
| 1338 | `negate(arg)` | Unary negation |
| 1348 | `multiply_floats(args, fresult)` | Multiplication with float promotion |
| 1357 | `divide_floats(args, fresult)` | Division with float promotion |
| 1368 | `remmod(args, mod)` | Remainder/modulo |
| 1408 | `intpower(base, exp)` | Integer exponentiation |
| 1420 | `testargument(args)` | Evaluate test expression |
| 1430 | `delassoc(key, alist)` | Delete association |
| 1448 | `nextpower2(n)` | Round up to power of 2 |

---

## 14. Arrays

uLisp arrays are stored as a flat block of objects in the workspace with dimension metadata. `(make-array '(3 4))` allocates 12 consecutive objects; `(aref array 1 2)` computes the linear index from the subscripts. Arrays can also be **bit arrays** — `(make-array 8 t)` creates an 8-element bit array packed into a single integer where each bit is individually addressable. The `rslice`/`pslice` functions handle reading and printing multi-dimensional arrays recursively, processing one dimension at a time with proper bracket notation.

| Line | Function | Description |
|-----:|----------|-------------|
| 1454 | `buildarray(n, s, def)` | Allocate array storage |
| 1464 | `makearray(dims, def, bitp)` | Create array from dimensions |
| 1486 | `arrayref(array, index, size)` | Get element pointer |
| 1496 | `getarray(array, subs, env, bit)` | Subscript access |
| 1520 | `rslice(array, size, slice, dims, args)` | Read array slice |
| 1533 | `readarray(d, args)` | Read array from input |
| 1550 | `readbitarray(gfun)` | Read bit array |
| 1577 | `pslice(array, size, slice, dims, pfun, bitp)` | Print array slice |
| 1594 | `printarray(array, pfun)` | Print array |

---

## 15. Strings

Strings in uLisp are stored as linked lists of objects, each holding a 32-bit packed word (the same radix-40 encoding used for long symbol names). A 10-character string uses about 2 objects (16 bytes) — not memory-efficient, but it works with the existing garbage collector without special allocation. `readstring()` builds the packed list from input characters; `printstring()` unpacks it for output. `cstring()` converts a Lisp string to a C `char*` for interfacing with Arduino libraries.

| Line | Function | Description |
|-----:|----------|-------------|
| 1616 | `indent(spaces, ch, pfun)` | Print indentation |
| 1620 | `startstring()` | Begin string capture |
| 1627 | `princtostring(arg)` | Convert object to string |
| 1633 | `buildstring(ch, tail)` | Append char to string |
| 1649 | `copystring(arg)` | Copy string |
| 1663 | `readstring(delim, esc, gfun)` | Read string from input |
| 1676 | `stringlength(form)` | String character count |
| 1689 | `getcharplace(string, n, shift)` | Get character position in packed string |
| 1702 | `nthchar(string, n)` | Get nth character |
| 1709 | `gstr()` / `pstr(c)` | String I/O callbacks |
| 1719 | `lispstring(s)` | Create Lisp string from C string |
| 1731 | `stringcompare(args, lt, gt, eq)` | String comparison engine |
| 1813 | `cstring(form, buffer, buflen)` | Lisp string → C string |
| 1831 | `iptostring(ip)` | IP address → dotted string |
| 1842 | `ipstring(form)` | Dotted string → IP address |

---

## 16. Environment & Closures

This is where the interpreter's semantics live. An **environment** is an association list — a linked list of `(symbol . value)` pairs. When `eval` encounters a symbol, `findpair(var, env)` searches the local environment first, then the global environment.

A **closure** captures the current environment alongside a function body. When you evaluate `(lambda (x) (+ x 1))`, uLisp wraps it as `(closure env-copy (x) (+ x 1))` where `env-copy` is a snapshot of the current bindings. `closure()` (line 1894) binds parameters to arguments, extends the captured environment, and returns the body form for evaluation.

**Tail-call optimization** is the key trick in `eval()`: instead of `return eval(form, env)` (which nests C stack frames), it sets the new `form` and `env` and does `goto EVAL`. This means tail-recursive functions run in constant stack space.

**`place(args, env, bit)`** (line 1981) implements `setf` — generalized assignment. `(setf (car x) 42)` modifies a cons cell; `(setf (aref a 3) 99)` modifies an array element. It returns a pointer to the storage location.

| Line | Function | Description |
|-----:|----------|-------------|
| 1861 | `value(n, env)` | Look up variable value |
| 1874 | `findpair(var, env)` | Find binding pair |
| 1881 | `boundp(var, env)` | Is variable bound? |
| 1886 | `findvalue(var, env)` | Look up or error |
| 1894 | `closure(tc, name, fn, args, env)` | Create closure / tail-call |
| 1957 | `apply(function, args, env)` | Apply function to arguments |
| 1981 | `place(args, env, bit)` | Generalized variable (setf place) |

### List Accessors

| Line | Function | Description |
|-----:|----------|-------------|
| 2028 | `carx(arg)` | Car with error check |
| 2034 | `cdrx(arg)` | Cdr with error check |
| 2040 | `cxxxr(args, pattern)` | Generalized c[ad]{2,3}r |
| 2051 | `mapcl(args, env, mapl)` | Map that modifies in place |
| 2079 | `mapcarfun(result, tail)` | Map append helper |
| 2084 | `mapcanfun(result, tail)` | Map nconc helper |
| 2092 | `mapcarcan(args, env, fun, maplist)` | General map engine |
| 2122 | `dobody(args, env, star)` | do/dolist/dotimes body evaluator |

---

## 17. I/O Streams & Serial

uLisp abstracts all I/O through two function pointer types: **`gfun_t`** (reads one character, returns `int`) and **`pfun_t`** (writes one character, takes `char`). Every I/O channel — serial, I2C, SPI, SD card, WiFi, display, string buffer — provides a pair of these functions. The reader and printer take a `gfun_t` or `pfun_t` parameter, so they work with any channel without knowing what's behind it.

The stream dispatch tables (lines 2356–2476) map small integer addresses to the correct read/write pair. The special forms `with-serial`, `with-i2c`, `with-spi`, and `with-sd-card` bind a stream for the duration of their body, then close it — a resource-management pattern like `with-open` in other Lisps.

The tone generation functions (lines 2555–2593) configure the RP2040's PWM hardware to drive the PicoCalc's speaker. `playnote(pin, note, octave)` maps musical note names to frequencies.

### I2C
| Line | Function | Description |
|-----:|----------|-------------|
| 2196 | `I2Cinit(port, enablePullup)` | Initialize I2C |
| 2201 | `I2Cread(port)` | Read byte from I2C |
| 2205 | `I2Cwrite(port, data)` | Write byte to I2C |
| 2209 | `I2Cstart(port, addr, read)` | Start I2C transaction |
| 2220 | `I2Crestart(port, addr, read)` | Restart I2C transaction |
| 2227 | `I2Cstop(port, read)` | Stop I2C transaction |

### Stream Write Callbacks (pfun_t)
| Line | Callback | Target |
|-----:|----------|--------|
| 2265 | `spiwrite` | SPI |
| 2269 | `i2cwrite` | I2C (Wire) |
| 2274 | `serial1write` | Serial1 |
| 2278 | `serial2write` | Serial2 |
| 2285 | `SDwrite` | SD card file |
| 2290 | `WiFiwrite` | WiFi client |
| 2293 | `gfxwrite` | TFT display |

### Stream Read Callbacks (gfun_t)
| Line | Callback | Source |
|-----:|----------|--------|
| 2296 | `spiread` | SPI |
| 2300 | `i2cread` | I2C |
| 2305 | `serial3read` | Serial3 |
| 2308 | `serial2read` | Serial2 |
| 2311 | `serial1read` | Serial1 |
| 2314 | `SDread` | SD card file |
| 2318 | `WiFiread` | WiFi client |

### Stream Dispatch
| Line | Function | Description |
|-----:|----------|-------------|
| 2321 | `serialbegin(addr, baud)` | Open serial port |
| 2338 | `serialend(addr)` | Close serial port |
| 2356–2476 | `pfun_*` / `gfun_*` dispatchers | Route stream address → callback |
| 2501 | `streamtable(n)` | Get stream table entry |
| 2506 | `pstreamfun(args)` | Resolve write function for stream |
| 2520 | `gstreamfun(args)` | Resolve read function for stream |

### Analog & Tone
| Line | Function | Description |
|-----:|----------|-------------|
| 2536 | `checkanalogread(pin)` | Validate analog input pin |
| 2544 | `checkanalogwrite(pin)` | Validate analog output pin |
| 2555 | `play_tone(freq)` | PWM tone generation |
| 2570 | `play_tone_off()` | Stop PWM tone |
| 2580 | `playnote(pin, note, octave)` | Play musical note |
| 2588 | `nonote(pin)` | Stop note |

### Sleep
| Line | Function | Description |
|-----:|----------|-------------|
| 2595 | `initsleep()` | (No-op on RP2040) |
| 2597 | `doze(secs)` | Low-power sleep |

---

## 18. Pretty Printing

The pretty printer formats Lisp output to fit within the screen width. `superprint(form, lm, pfun)` (line 2644) measures each sub-form's width using `atomwidth()` and `subwidth()`. If the entire form fits on one line, it prints inline; otherwise it breaks after the opening bracket, indents, and prints each element on its own line. `pcount` tracks the current column position.

| Line | Function | Description |
|-----:|----------|-------------|
| 2608 | `pcount(c)` | Column counter |
| 2613 | `atomwidth(obj)` | Width of printed atom |
| 2619 | `basewidth(obj, base)` | Width in given base |
| 2625 | `quoted(obj)` | Is it quoted? |
| 2629 | `subwidth(obj, w)` | Width of subform |
| 2635 | `subwidthlist(form, w)` | Width of form list |
| 2644 | `superprint(form, lm, pfun)` | Pretty-print with line breaks |

---

## 19. Editor & ARM Assembler

**`edit(fun)`** (line 2680) is a line editor that lets you redefine functions interactively — display the source, navigate with arrow keys, modify, and save.

**`defcode`** is a unique uLisp feature: it lets you write ARM machine code inline in Lisp. The two-pass assembler at `assemble()` (line 2728) takes a list of assembly instructions and emits actual ARM opcodes into a reserved code area. `call()` (line 2699) then jumps to this machine code with your arguments. This is used for performance-critical operations where the interpreter is too slow — bit manipulation, tight loops, and direct hardware access.

| Line | Function | Description |
|-----:|----------|-------------|
| 2680 | `edit(fun)` | Line editor for function definitions |
| 2699 | `call(entry, nargs, args, env)` | Call ARM machine code |
| 2716 | `putcode(arg, origin, pc)` | Emit machine code instruction |
| 2728 | `assemble(pass, origin, entries, env, pcpair)` | Two-pass ARM assembler |

---

## 20. Special Forms (sp_)

Special forms are the bones of the language. They differ from regular functions in one critical way: **they do not evaluate their arguments**. This is what makes them special. `if` doesn't evaluate both branches — it picks one. `setq` doesn't evaluate the variable name — it uses it as a destination. `defun` doesn't evaluate the parameter list — it treats it as a specification.

`sp_defun` creates a function by building a lambda and binding it in the global environment. `sp_loop` implements a mini-language with `for`, `while`, `do`, `finally`, and `return` clauses. The `with-*` forms (`with-serial`, `with-i2c`, `with-spi`, `with-sd-card`, `with-gfx`, `with-client`) open a resource, evaluate a body, then close it — a resource-management pattern. `sp_setf` implements generalized assignment through the `place()` mechanism.

| Line | Lisp Name | Function | Description |
|-----:|-----------|----------|-------------|
| 2779 | `quote` | `sp_quote` | Return argument unevaluated |
| 2784 | `or` | `sp_or` | Short-circuit or |
| 2793 | `defun` | `sp_defun` | Define function |
| 2804 | `defvar` | `sp_defvar` | Define variable |
| 2816 | `setq` | `sp_setq` | Set variable |
| 2828 | `loop` | `sp_loop` | Loop construct |
| 2844 | `push` | `sp_push` | Push onto list |
| 2853 | `pop` | `sp_pop` | Pop from list |
| 2867 | `incf` | `sp_incf` | Increment |
| 2910 | `decf` | `sp_decf` | Decrement |
| 2953 | `setf` | `sp_setf` | Generalized assignment |
| 2970 | `dolist` | `sp_dolist` | Iterate over list |
| 3000 | `dotimes` | `sp_dotimes` | Iterate N times |
| 3027 | `do` | `sp_do` | General do loop |
| 3031 | `do*` | `sp_dostar` | Sequential do loop |
| 3035 | `trace` | `sp_trace` | Trace functions |
| 3051 | `untrace` | `sp_untrace` | Untrace functions |
| 3071 | `for-millis` | `sp_formillis` | Timed loop |
| 3085 | `time` | `sp_time` | Measure execution time |
| 3103 | `with-output-to-string` | `sp_withoutputtostring` | Capture output to string |
| 3116 | `with-serial` | `sp_withserial` | Open serial port scope |
| 3132 | `with-i2c` | `sp_withi2c` | Open I2C scope |
| 3162 | `with-spi` | `sp_withspi` | Open SPI scope |
| 3205 | `with-sd-card` | `sp_withsdcard` | Open SD card scope |
| 3245 | `defcode` | `sp_defcode` | Define ARM machine code |
| 4981 | `?` | `sp_help` | Help system |
| 5010 | `unwind-protect` | `sp_unwindprotect` | Exception handling |
| 5040 | `ignore-errors` | `sp_ignoreerrors` | Suppress errors |
| 5067 | `error` | `sp_error` | Signal error |
| 5108 | `with-client` | `sp_withclient` | WiFi client scope |
| 5238 | `with-gfx` | `sp_withgfx` | Graphics output scope |

---

## 21. Tail-Call Forms (tf_)

Tail forms are like special forms but with one additional property: **their last expression is evaluated as a tail call**. Instead of `eval()` recursing in C (consuming stack frames), it loops back via `goto EVAL` in constant stack space. `tf_progn` evaluates each form in sequence and returns the last. `tf_if` picks a branch. `tf_cond` tests clauses in order. `tf_and` short-circuits on the first false value. `tf_case` dispatches on a key value.

| Line | Lisp Name | Function | Description |
|-----:|-----------|----------|-------------|
| 3350 | `progn` | `tf_progn` | Sequential evaluation |
| 3362 | `if` | `tf_if` | Conditional |
| 3369 | `cond` | `tf_cond` | Multi-branch conditional |
| 3383 | `when` | `tf_when` | Conditional execution |
| 3389 | `unless` | `tf_unless` | Negated conditional |
| 3395 | `case` | `tf_case` | Case/switch |
| 3414 | `and` | `tf_and` | Short-circuit and |

---

## 22. Built-in Functions (fn_)

These are the standard library — over 200 functions callable from Lisp, each following the same signature `object *fn_name(object *args, object *env)` where `args` is a list of already-evaluated arguments. They are organized by category in the source: list operations (car/cdr/cons/append/mapcar), arithmetic (+/-/*// comparisons and transcendental functions), characters and strings (char/string=/subseq/concatenate), bitwise operations (logand/logior/ash), I/O (read/print/prin1/princ), hardware (pinmode/digitalwrite/analogread), graphics (draw-pixel/draw-line/fill-rect/draw-circle — PicoCalc-specific), and PicoCalc-specific extensions (get-key/read-pixel/save-bmp).
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 3427 | `not` | Logical negation |
| 3432 | `cons` | Create pair |
| 3437 | `atom` | Is it an atom? |
| 3442 | `listp` | Is it a list? |
| 3447 | `consp` | Is it a cons? |
| 3452 | `symbolp` | Is it a symbol? |
| 3458 | `arrayp` | Is it an array? |
| 3463 | `boundp` | Is variable bound? |
| 3467 | `keywordp` | Is it a keyword? |
| 3476 | `set` | Set variable |
| 3488 | `streamp` | Is it a stream? |
| 3494 | `eq` | Pointer equality |
| 3499 | `equal` | Structural equality |

### List Operations
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 3506–3571 | `car` … `cdddr` | All c[ad]{1,3}r combinations |
| 3576 | `length` | List/string length |
| 3585 | `array-dimensions` | Get array dimensions |
| 3593 | `list` | Create list from args |
| 3598 | `copy-list` | Shallow copy |
| 3633 | `reverse` | Reverse list |
| 3645 | `nth` | Nth element |
| 3659 | `aref` | Array element access |
| 3669 | `assoc` | Association list lookup |
| 3684 | `member` | List membership |
| 3697 | `apply` | Apply function |
| 3710 | `funcall` | Call function |
| 3714 | `append` | Concatenate lists |
| 3734–3754 | `mapc`, `mapl`, `mapcar`, `mapcan`, `maplist`, `mapcon` | Map functions |

### Arithmetic
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 3760 | `+` | Addition |
| 3777 | `-` | Subtraction |
| 3801 | `*` | Multiplication |
| 3817 | `/` | Division |
| 3856 | `mod` | Modulo |
| 3861 | `rem` | Remainder |
| 3866 | `1+` | Increment |
| 3878 | `1-` | Decrement |
| 3890 | `abs` | Absolute value |
| 3902 | `random` | Random number |
| 3911 | `max` | Maximum |
| 3925 | `min` | Minimum |

### Numeric Comparison
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 3941 | `/=` | Not equal |
| 3959 | `=` | Numeric equal |
| 3964 | `<` | Less than |
| 3969 | `<=` | Less or equal |
| 3974 | `>` | Greater than |
| 3979 | `>=` | Greater or equal |
| 3984 | `plusp` | Positive? |
| 3993 | `minusp` | Negative? |
| 4002 | `zerop` | Zero? |
| 4011 | `oddp` | Odd? |
| 4017 | `evenp` | Even? |
| 4025 | `integerp` | Is integer? |
| 4030 | `numberp` | Is number? |
| 4038 | `float` | Convert to float |
| 4044 | `floatp` | Is float? |

### Transcendental Functions
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4049 | `sin` | Sine |
| 4054 | `cos` | Cosine |
| 4059 | `tan` | Tangent |
| 4064 | `asin` | Arc sine |
| 4069 | `acos` | Arc cosine |
| 4074 | `atan` | Arc tangent |
| 4083 | `sinh` | Hyperbolic sine |
| 4088 | `cosh` | Hyperbolic cosine |
| 4093 | `tanh` | Hyperbolic tangent |
| 4098 | `exp` | Exponential |
| 4103 | `sqrt` | Square root |
| 4108 | `log` | Natural log |
| 4117 | `expt` | Exponentiation |
| 4131 | `ceiling` | Round up |
| 4147 | `floor` | Round down |
| 4163 | `truncate` | Truncate |
| 4177 | `round` | Round |

### Characters & Strings
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4187 | `char` | Character in string |
| 4197 | `char-code` | Char → integer |
| 4202 | `code-char` | Integer → char |
| 4207 | `characterp` | Is character? |
| 4214 | `stringp` | Is string? |
| 4219 | `string=` | String equal |
| 4225 | `string<` | String less than |
| 4231 | `string>` | String greater than |
| 4237 | `string/=` | String not equal |
| 4243 | `string<=` | String less or equal |
| 4249 | `string>=` | String greater or equal |
| 4255 | `sort` | Sort list |
| 4284 | `string` | Convert to string |
| 4288 | `concatenate` | Concatenate strings |
| 4312 | `subseq` | Substring |
| 4344 | `search` | String search |
| 4380 | `read-from-string` | Parse from string |
| 4390 | `princ-to-string` | Print to string (no escapes) |
| 4395 | `prin1-to-string` | Print to string (with escapes) |

### Bitwise Operations
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4405 | `logand` | Bitwise AND |
| 4415 | `logior` | Bitwise OR |
| 4425 | `logxor` | Bitwise XOR |
| 4435 | `lognot` | Bitwise NOT |
| 4441 | `ash` | Arithmetic shift |
| 4449 | `logbitp` | Test bit |

### I/O & System
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4458 | `eval` | Evaluate expression |
| 4462 | `return` | Return from loop/block |
| 4468 | `globals` | List global variables |
| 4480 | `locals` | List local variables |
| 4485 | `makunbound` | Unbind variable |
| 4493 | `break` | Enter debugger |
| 4502 | `read` | Read expression |
| 4508 | `prin1` | Print (readable) |
| 4516 | `print` | Print + newline |
| 4526 | `princ` | Print (human-readable) |
| 4534 | `terpri` | Print newline |
| 4541 | `read-byte` | Read byte from stream |
| 4549 | `read-line` | Read line from stream |
| 4556 | `write-byte` | Write byte to stream |
| 4565 | `write-string` | Write string to stream |
| 4576 | `write-line` | Write line to stream |
| 4588 | `restart-i2c` | Restart I2C transaction |
| 4609 | `gc` | Run garbage collector |
| 4624 | `room` | Report free space |
| 4629 | `backtrace` | Print backtrace |
| 4636 | `save-image` | Save workspace |
| 4641 | `load-image` | Load workspace |
| 4647 | `cls` | Clear screen (PicoCalc) |

### Hardware Interface
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4655 | `pinmode` | Set pin mode |
| 4674 | `digitalread` | Read digital pin |
| 4683 | `digitalwrite` | Write digital pin |
| 4698 | `analogread` | Read analog pin |
| 4710 | `analogreference` | Set analog reference |
| 4728 | `analogreadresolution` | Set ADC resolution |
| 4741 | `analogwrite` | Write analog (PWM) |
| 4753 | `analogwriteresolution` | Set DAC resolution |
| 4760 | `delay` | Millisecond delay |
| 4770 | `millis` | Milliseconds since boot |
| 4775 | `sleep` | Low-power sleep |
| 4782 | `note` | Play musical note |

### Editor & Debugging
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 4797 | `register` | Read/write register |
| 4810 | `edit` | Edit function |
| 4821 | `pprint` | Pretty print |
| 4834 | `pprintall` | Pretty print all |
| 4863 | `format` | Formatted output (Common Lisp style) |
| 4941 | `require` | Load from library |
| 4965 | `list-library` | List available libraries |
| 4993 | `documentation` | Get function documentation |
| 4997 | `apropos` | Search symbols |
| 5003 | `apropos-list` | Search symbols (return list) |

### WiFi Networking
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 5082 | `directory` | List SD directory |
| 5142 | `available` | WiFi data available? |
| 5154 | `wifi-server` | Start WiFi server |
| 5166 | `wifi-softap` | Start software AP |
| 5190 | `connected` | WiFi connected? |
| 5202 | `wifi-localip` | Get local IP |
| 5213 | `wifi-connect` | Connect to WiFi |

### Graphics (PicoCalc TFT Display)
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 5254 | `draw-pixel` | Set pixel |
| 5266 | `draw-line` | Draw line |
| 5279 | `draw-rect` | Draw rectangle outline |
| 5292 | `fill-rect` | Draw filled rectangle |
| 5305 | `draw-circle` | Draw circle outline |
| 5318 | `fill-circle` | Draw filled circle |
| 5331 | `draw-round-rect` | Draw rounded rectangle |
| 5344 | `fill-round-rect` | Draw filled rounded rectangle |
| 5357 | `draw-triangle` | Draw triangle outline |
| 5370 | `fill-triangle` | Draw filled triangle |
| 5383 | `draw-char` | Draw character |
| 5405 | `set-cursor` | Set text cursor |
| 5415 | `set-text-color` | Set text color |
| 5426 | `set-text-size` | Set text scale |
| 5436 | `set-text-wrap` | Set text wrap |
| 5446 | `fill-screen` | Clear screen with color |
| 5458 | `set-rotation` | Set display rotation |
| 5468 | `invert-display` | Invert display |

### PicoCalc-Specific
| Line | Lisp Name | Description |
|-----:|-----------|-------------|
| 5480 | — | `getKey()` — raw keyboard read |
| 5494 | `get-key` | Read keyboard input |
| 5499 | `read-pixel` | Read pixel from display |
| 5510–5515 | — | `writeTwo` / `writeFour` — BMP helpers |
| 5520 | — | `savebmp(arg)` — save BMP screenshot to SD |
| 5563 | `save-bmp` | Save screenshot |

---

## 23. Data Tables (Symbol Names, Docs, Lookup)

Three large `const` arrays form the bridge between the symbol system and the C implementation:

- **Symbol name strings** (lines 5565–5833): `const char string0[]` through `string213[]` — the C string literals for every built-in symbol (`"nil"`, `"t"`, `"car"`, `"+"`, etc.).
- **Documentation strings** (lines 5836–6415): `const char doc0[]` through `doc248[]` — one-liners for `(documentation 'car)` and the `?` help command.
- **`lookup_table[]`** (lines 6417–6683): The master dispatch table. Each entry has a name string, a function pointer (NULL for constants), a packed min/max-args byte, and a doc pointer. When `eval()` encounters `(car x)`, it looks up `car`'s builtin index, indexes into this table, and calls the `fn_car` function pointer.

The helper functions below the table (`lookupbuiltin`, `lookupfn`, `getminmax`, `lookupdoc`) are thin accessors used by `eval()` and the error/reporting system.

| Lines | Description |
|------:|-------------|
| 5565–5833 | `const char string0[]` … `string213[]` — Built-in symbol name strings |
| 5836–6415 | `const char doc0[]` … `doc248[]` — Documentation strings for each builtin |
| 6417–6683 | `lookup_table[]` — Master dispatch table: name → function pointer, min/max args, type |
| 6691 | `table(n)` | Index into lookup table |
| 6695 | `tablesize(n)` | Size of lookup table segment |
| 6702 | `lookupbuiltin(c)` | Name string → builtin index |
| 6715 | `lookupfn(name)` | Builtin → function pointer |
| 6720 | `getminmax(name)` | Get min/max argument counts |
| 6725 | `checkminmax(name, nargs)` | Validate argument count |
| 6732 | `lookupdoc(name)` | Builtin → documentation string |
| 6737 | `findsubstring(part, name)` | Search doc string |
| 6742 | `testescape()` | Check for ESC key (abort) |
| 6756 | `colonp(name)` | Is symbol a keyword? |
| 6763 | `keywordp(obj)` | Is object a keyword? |
| 6772 | `backtrace(name)` | Print call backtrace |

---

## 24. Evaluator

**`eval(object *form, object *env)`** (line 6788) is the function that makes the whole system go — 187 lines that implement the Lisp eval/apply loop. The core is a `goto EVAL` loop that implements tail-call optimization. Here is what happens at each stage:

1. **Stack check** — if the C stack is too deep, raise "stack overflow"
2. **GC trigger** — if free space < 1/16 of workspace, run garbage collection
3. **Escape check** — if the user pressed Escape, abort
4. **Self-evaluating atoms** — numbers, floats, chars, strings, arrays evaluate to themselves
5. **Symbol lookup** — search local env, then global env, then check if builtin
6. **Let/Lambda** — special handling that creates environments and closures
7. **Special forms** — call `sp_*` function directly (not tail-callable)
8. **Tail forms** — call `tf_*` function, then `goto EVAL` with the result
9. **Evaluate arguments** — for regular functions, eval each arg left-to-right
10. **Builtin functions** — look up `fn_*` pointer in the lookup table, call it
11. **Closures** — bind parameters via `closure()`, then `goto EVAL` with the body

Steps 8 and 11 are the tail-call optimization — instead of nesting C stack frames, the function loops. This is what makes deeply recursive Lisp code practical on a microcontroller.

| Line | Function | Description |
|-----:|----------|-------------|
| 6788 | `eval(form, env)` | **The heart of uLisp** — the eval/apply loop. Dispatches on form type: symbol lookup, self-evaluating types, special forms (by index), tail-forms, builtin functions, user-defined closures. |

This is the 190-line function that drives everything. Key dispatch points within:

| Line | Subsection | Description |
|-----:|------------|-------------|
| ~6796 | Symbol | Variable lookup |
| ~6810 | Self-eval | Numbers, floats, chars, strings, arrays |
| ~6825 | Symbol `t`, `nil` | Self-evaluating constants |
| ~6835 | Builtin check | Dispatch special/tail/function forms |
| ~6850 | Special forms | `sp_*` functions |
| ~6880 | Tail forms | `tf_*` functions |
| ~6920 | Builtin functions | `fn_*` functions |
| ~6940 | Closures | User-defined function calls |
| ~6960 | Error | "not a function" |

---

## 25. Printer (Output)

The printer converts Lisp objects back into characters. `pserial(c)` (line 6976) is the default output — it writes a character to the serial port and to the PicoCalc display. Every other print function eventually calls a `pfun_t` callback like this.

The printing is layered: `printobject()` dispatches on type (numbers → `pint`, symbols → `psymbol`, strings → `printstring`, lists → `plist`). `pint()` and `pintbase()` convert integers to decimal/hex/octal. `pfloat()` converts floats — `pmantissa()` handles digit-by-digit conversion without `sprintf` to save code size. `pln()` prints a newline; `pfl()` prints a fresh line (newline only if not already at column 0).

| Line | Function | Description |
|-----:|----------|-------------|
| 6976 | `pserial(c)` | Write char to serial/REPL output |
| 6988 | `pcharacter(c, pfun)` | Print character (with escape) |
| 7001 | `pstring(s, pfun)` | Print C string |
| 7005 | `plispstring(form, pfun)` | Print Lisp string |
| 7009 | `plispstr(name, pfun)` | Print symbol name |
| 7022 | `printstring(form, pfun)` | Print string with escapes |
| 7028 | `pbuiltin(name, pfun)` | Print builtin name |
| 7038 | `pradix40(name, pfun)` | Print radix-40 symbol |
| 7048 | `printsymbol(form, pfun)` | Print symbol |
| 7052 | `psymbol(name, pfun)` | Print symbol by name |
| 7062 | `pfstring(s, pfun)` | Print format string |
| 7070 | `pint(i, pfun)` | Print integer (decimal) |
| 7076 | `pintbase(i, base, pfun)` | Print integer in base |
| 7086 | `printhex4(i, pfun)` | Print 4-digit hex |
| 7096 | `pmantissa(f, pfun)` | Print float mantissa |
| 7123 | `pfloat(f, pfun)` | Print float |
| 7144 | `pln(pfun)` | Print newline |
| 7148 | `pfl(pfun)` | Print fresh line |
| 7152 | `plist(form, pfun)` | Print list |
| 7169 | `pstream(form, pfun)` | Print stream |
| 7180 | `printobject(form, pfun)` | Print any object (readable) |
| 7195 | `prin1object(form, pfun)` | Print any object (compact) |

---

## 26. Display & Keyboard (PicoCalc-specific)

These functions implement the PicoCalc's on-display REPL — a terminal emulator that shows the Lisp read-eval-print loop on the TFT screen with keyboard input from the PCKeyboard I2C module.

**`Display(c)`** (line 7263) is the terminal emulator — an 80-line function that maintains a cursor position (line, column) and interprets control characters: regular chars are plotted at the cursor, `\n` moves to the next line, backspace moves back, form-feed clears the screen. When the cursor reaches the bottom, **`ScrollDisplay()`** scrolls everything up by one line, redrawing only changed characters.

**`gserial()`** (line 7464) is the main input function — it reads keys from the keyboard, handles special keys (arrows, Enter, Escape, Tab for auto-completion), and feeds characters to the reader. **`ProcessKey(c)`** (line 7393) dispatches raw key codes to actions. The keyboard buffer uses circular indices (`WritePtr`/`ReadPtr`) to decouple the interrupt-driven key arrival from the reader's consumption.

| Line | Function | Description |
|-----:|----------|-------------|
| 7221 | `PlotChar(ch, line, column)` | Render character at grid position |
| 7234 | `ScrollDisplay()` | Scroll terminal up one line |
| 7263 | `Display(c)` | **Terminal emulator** — handles characters, cursor, scrolling, escape sequences |
| 7319 | `initkybd()` | Initialize keyboard state |
| 7329 | `autoComplete()` | Tab-completion for symbols |
| 7383 | `Highlight(p, invert)` | Invert text region (selection) |
| 7393 | `ProcessKey(c)` | **Key dispatcher** — maps raw keys to editor actions |
| 7213 | `WritePtr`, `ReadPtr`, `LastWritePtr` | Circular buffer pointers |
| 7216 | `KybdAvailable` | Keyboard data ready flag |
| 7217 | `Scroll` | Scroll state |

---

## 27. Reader (Input/Parser)

The reader converts a stream of characters into Lisp objects — a recursive-descent parser. **`read(gfun)`** (line 7680) reads one character at a time, skipping whitespace, then dispatches: `(` calls `readrest()` to build a list, `'` wraps in `(quote ...)`, `"` reads a string literal, `#` handles special syntaxes (`#\char`, `#x1F`, `#(1 2 3)`), and everything else is parsed as a number (falling back to a symbol name if that fails). **`nextitem(gfun)`** (line 7512) is the workhorse that handles character-by-character parsing including escape sequences, negative numbers, and dotted-pair notation.

| Line | Function | Description |
|-----:|----------|-------------|
| 7440 | `glibrary()` | Read char from library |
| 7445 | `loadfromlibrary(env)` | Load Lisp library |
| 7456 | `gserial_flush()` | Flush serial input buffer |
| 7464 | `gserial()` | **Main input reader** — reads from keyboard, handles escape sequences, autocomplete |
| 7512 | `nextitem(gfun)` | Read next token from input |
| 7637 | `readrest(gfun)` | Read list body |
| 7664 | `glast()` | Peek last character |
| 7673 | `readmain(gfun)` | Read expression (entry) |
| 7680 | `read(gfun)` | Top-level read (handles reader macros) |

---

## 28. Setup, REPL & Entry Points

**`setup()`** (line 7706) is the Arduino entry point, called once on boot: opens serial, initializes the workspace free list, binds `t` and `nil` in the global environment, initializes the TFT display and I2C keyboard, and prints the "uLisp 4.8f" banner.

**`loop()`** (line 7766) is called repeatedly by the Arduino framework. It sets up the `setjmp` error recovery point, optionally autoruns a saved image, then calls `repl()`.

**`repl(env)`** (line 7724) is the Read-Eval-Print Loop: `for (;;) { line = readmain(gserial); result = eval(line, env); printobject(result, pserial); }`. It loops forever. The only way out is an error (which longjmps back to `loop()`) or power-off.

**`ulisperror()`** (line 7779) is the cleanup function called after any error — it drains buffered input, resets flags, closes files, and prepares for a fresh REPL restart.

| Line | Function | Description |
|-----:|----------|-------------|
| 7691 | `initenv()` | Initialize global environment (bind `t`, `nil`, features) |
| 7696 | `initgfx()` | Initialize TFT display, keyboard, sound |
| 7706 | `setup()` | **Arduino entry point** — init display, env, load image, print banner |
| 7724 | `repl(env)` | **Read-Eval-Print Loop** — the main interactive loop |
| 7766 | `loop()` | **Arduino main loop** — calls `repl()` or `ulisperror()` |
| 7779 | `ulisperror()` | Error recovery — longjmp target, restart REPL |

---

## Quick Reference: Naming Conventions

| Prefix | Meaning | Example |
|--------|---------|---------|
| `sp_` | Special form (no argument eval) | `sp_if`, `sp_defun`, `sp_loop` |
| `tf_` | Tail-call form | `tf_progn`, `tf_cond`, `tf_and` |
| `fn_` | Built-in function | `fn_car`, `fn_add`, `fn_drawline` |
| `p*` | Print/output function | `pserial`, `pint`, `pln`, `pfloat` |
| `g*` | Input/read function | `gserial`, `gstr` |
| `check*` | Type validator | `checkinteger`, `checkstring` |
| `Flash*` | Flash memory I/O | `FlashWrite`, `FlashRead32` |
| `SD*` / `FS*` | SD card / LittleFS I/O | `SDBegin`, `FSWrite32` |
| `I2C*` | I2C bus operations | `I2Cstart`, `I2Cread` |
