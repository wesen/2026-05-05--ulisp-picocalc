---
Title: 'Understanding WITH-OUTPUT-TO-STRING in uLisp PicoCalc'
Ticket: ulisp-picocalc
Status: active
Topics:
    - embedded
    - lisp
    - rp2040
    - picocalc
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ulisp-picocalc/ulisp-picocalc.ino
      Note: 'Primary source file containing sp_withoutputtostring (line 3103), stream dispatch table, and string building primitives'
---

# Understanding WITH-OUTPUT-TO-STRING in uLisp PicoCalc

## Goal

This document explains how `with-output-to-string` works in the uLisp PicoCalc firmware, from its conceptual purpose in Lisp down to the exact C implementation that makes it run on a microcontroller with 264 KB of RAM. By the end, you will understand not only how to use it, but also how it connects to the broader stream architecture of uLisp, and why its behavior differs subtly from Common Lisp.

## Context

uLisp is an embedded Lisp interpreter that runs on microcontrollers. The PicoCalc version targets the Raspberry Pi RP2040, packing an entire interpreter—reader, evaluator, printer, garbage collector, and hardware drivers—into a single 7,793-line C++ file. On this platform, every design decision is a tradeoff between memory, speed, and expressiveness. `with-output-to-string` is a perfect case study because it sits at the intersection of three systems: the Lisp special-form evaluator, the stream abstraction, and the packed-string memory representation.

---

## 1. The Problem: Capturing Output as Data

Lisp systems are built around the read-eval-print loop. You type an expression, the evaluator computes a result, and the printer renders that result as text. But sometimes you need the printed text itself as a value—not on the screen, but as a string you can pass to another function, store in a variable, or write to a file.

Consider formatting a number for display:

```lisp
(format nil "The answer is ~a" 42)
```

In Common Lisp, `format` with a `nil` destination returns the formatted string. In uLisp, `format` exists but does not support `nil` as a destination. Instead, uLisp provides `with-output-to-string`, a special form that redirects character output into a string buffer for the duration of its body.

The conceptual contract is simple: evaluate a sequence of forms, capture every character that would normally go to the screen, and return those characters as a single string object.

---

## 2. How It Looks from Lisp

Here is the canonical usage:

```lisp
(with-output-to-string (str)
  (princ "Hello" str)
  (princ " " str)
  (princ "World" str))
```

This returns the string `"Hello World"`.

The syntax is `(with-output-to-string (var) form*)`. The `var` is bound to a stream object. Inside the body, any output function that accepts an optional stream argument—`print`, `princ`, `prin1`, `write-string`, `write-line`, `terpri`—can be directed to `var`, and those characters are accumulated rather than displayed.

### A Common Mistake

Because uLisp is a subset of Common Lisp, it is easy to assume that `with-output-to-string` captures *all* output inside its body. It does not. If you write:

```lisp
(with-output-to-string (str)
  (print "Hello"))
```

The string `"Hello"` still appears on the screen. The `print` call uses the default output stream (`pserial`), not `str`. You must pass `str` explicitly:

```lisp
(with-output-to-string (str)
  (print "Hello" str))
```

This is the single most important behavioral difference between uLisp and Common Lisp. We will see exactly why this is the case when we examine the implementation.

---

## 3. The Stream Abstraction in uLisp

To understand `with-output-to-string`, you must first understand how uLisp abstracts I/O. Every I/O channel in uLisp—serial port, I2C bus, SPI device, SD card file, WiFi socket, display, and string buffer—is represented as a `STREAM` object.

### 3.1 The STREAM Object

A stream is an 8-byte `object` struct with `type = STREAM` and an `integer` field that encodes both the stream type and an address:

```c
object *stream (uint8_t streamtype, uint8_t address) {
  object *ptr = myalloc();
  ptr->type = STREAM;
  ptr->integer = streamtype<<8 | address;
  return ptr;
}
```

The stream type is drawn from an enumeration at line 2235:

```c
enum stream { SERIALSTREAM, I2CSTREAM, SPISTREAM, SDSTREAM, WIFISTREAM, STRINGSTREAM, GFXSTREAM };
```

A `STRINGSTREAM` (value 5) is the type used by `with-output-to-string`. Its address is always 0; there is no secondary identifier needed because the actual string data lives in global variables, not in the stream object itself.

### 3.2 The pfun_t and gfun_t Callbacks

uLisp decouples reading and writing from the underlying hardware through two function-pointer types:

- **`gfun_t`** — gets one character: `int gfun()`
- **`pfun_t`** — puts one character: `void pfun(char c)`

Every built-in printer function (`print`, `princ`, `pint`, `pfstring`) takes a `pfun_t` callback. When you call `(print x)`, uLisp resolves the default `pfun_t` to `pserial`, which writes to the serial port and the display. When you call `(print x str)`, uLisp resolves `str`'s stream type and returns the appropriate callback.

The dispatch happens in `pstreamfun` (line 2506):

```c
pfun_t pstreamfun (object *args) {
  nstream_t nstream = SERIALSTREAM;
  int address = 0;
  pfun_t pfun = pserial;
  if (args != NULL && first(args) != NULL) {
    int stream = isstream(first(args));
    nstream = stream>>8; address = stream & 0xFF;
  }
  bool n = nstream<USERSTREAMS;
  pstream_ptr_t streamfunction = streamtable(n?0:1)[n?nstream:nstream-USERSTREAMS].pfunptr;
  pfun = streamfunction(address);
  return pfun;
}
```

For a `STRINGSTREAM`, `streamtable` maps to `pfun_string` (line 2390):

```c
pfun_t pfun_string (uint8_t address) {
  (void) address;
  return pstr;
}
```

So when output is directed to a string stream, the character callback is `pstr`.

### 3.3 The Stream Dispatch Table

At line 2486, a static table maps each stream type to its name, write callback, and read callback:

```c
const stream_entry_t stream_table[] = {
  { serialstreamname, pfun_serial, gfun_serial },
  { i2cstreamname, pfun_i2c, gfun_i2c },
  { spistreamname, pfun_spi, gfun_spi },
  { sdstreamname, pfun_sd, gfun_sd },
  { wifistreamname, pfun_wifi, gfun_wifi },
  { stringstreamname, pfun_string, NULL },
  { gfxstreamname, pfun_gfx, NULL },
};
```

Notice that `STRINGSTREAM` has no read callback (`NULL`). This is because `with-output-to-string` is write-only; there is no corresponding `with-input-from-string` in this uLisp build.

---

## 4. The String Buffer: GlobalString and GlobalStringTail

Now we come to the mechanism that actually accumulates characters. uLisp stores the in-progress string in two global variables:

```c
object *GlobalString;     // Head of the string
object *GlobalStringTail; // Pointer to the last cell for append
```

### 4.1 Starting a String Capture

The function `startstring()` (line 1620) initializes the buffer:

```c
object *startstring () {
  object *string = newstring();
  GlobalString = string;
  GlobalStringTail = string;
  return string;
}
```

It creates an empty string object and points both globals at it. From this moment forward, any call to `pstr(c)` will append `c` to this buffer.

### 4.2 Appending Characters

The function `pstr()` (line 1715) is the write callback for string streams:

```c
void pstr (char c) {
  buildstring(c, &GlobalStringTail);
}
```

And `buildstring()` (line 1633) does the actual packing. uLisp strings are not contiguous C arrays; they are linked lists of objects, each holding up to four characters in a 32-bit packed word. The `buildstring` function walks the list and packs characters into the low bytes of each cell, allocating new cells from the free list as needed.

Why this representation? Because uLisp has a single, uniform memory model: everything is an 8-byte object in a flat workspace array. Strings do not get special allocation treatment. This makes the garbage collector simple—`markobject` and `sweep` handle strings the same way they handle cons cells—but it means string operations are slower than they would be with a contiguous buffer.

### 4.3 Retrieving the Result

When the body of `with-output-to-string` finishes, the function simply returns `GlobalString`. The string has been accumulating in place the entire time.

---

## 5. The Implementation of WITH-OUTPUT-TO-STRING

With all the pieces in place, the special form itself is remarkably compact. Here is `sp_withoutputtostring` at line 3103:

```c
object *sp_withoutputtostring (object *args, object *env) {
  object *params = checkarguments(args, 1, 1);
  object *var = first(params);
  object *pair = cons(var, stream(STRINGSTREAM, 0));
  push(pair,env);
  object *string = startstring();
  protect(string);
  object *forms = cdr(args);
  eval(tf_progn(forms,env), env);
  unprotect();
  return string;
}
```

Let us walk through this line by line.

### 5.1 Argument Checking

`checkarguments(args, 1, 1)` validates that the parameter list `(str)` contains exactly one element. The outer `args` is the full form `(with-output-to-string (str) form1 form2 ...)`, so `params` is `(str)` and `forms` is `(form1 form2 ...)`.

### 5.2 Environment Binding

```c
object *pair = cons(var, stream(STRINGSTREAM, 0));
push(pair,env);
```

This creates a new environment binding: `(str . <stringstream>)`. The variable `str` is now bound to a stream object of type `STRINGSTREAM`. This binding is local to the body of `with-output-to-string`; when the body finishes, `env` is discarded and the binding disappears.

### 5.3 Starting the String Buffer

```c
object *string = startstring();
protect(string);
```

`startstring()` initializes `GlobalString` and `GlobalStringTail`. `protect(string)` pushes the string onto the GC stack. This is critical: if the body of `with-output-to-string` triggers garbage collection (for example, by allocating a new list or calling `cons`), the garbage collector must not reclaim the string buffer that is still being built.

### 5.4 Evaluating the Body

```c
object *forms = cdr(args);
eval(tf_progn(forms,env), env);
```

The body forms are evaluated sequentially via `tf_progn`, which returns the last form as a tail call. The result of the body is discarded; `with-output-to-string` returns the accumulated string, not the value of the last expression. This is consistent with Common Lisp semantics.

### 5.5 Cleanup and Return

```c
unprotect();
return string;
```

`unprotect()` pops the GC root, and the function returns the string object. At this point, `GlobalString` and `GlobalStringTail` still point to the string, but they are globals of convenience; the caller receives the string as a first-class Lisp object.

---

## 6. Why Output Is Not Automatically Captured

We can now answer the puzzle from Section 2: why does `(with-output-to-string (str) (print "Hello"))` send output to the screen instead of the string?

The answer lies in how output functions resolve their destination. Consider `fn_print` (line 4516):

```c
object *fn_print (object *args, object *env) {
  pfun_t pfun = pstreamfun(cdr(args));
  object *result = eval(first(args), env);
  printobject(result, pfun);
  pfun('\n');
  return result;
}
```

`pstreamfun(cdr(args))` looks at the *optional stream argument* to `print`. If `cdr(args)` is `nil` (no stream provided), `pstreamfun` returns the default `pserial`. It does not look at the dynamic environment to find a "current output stream." uLisp has no concept of a dynamic `*standard-output*` binding.

Common Lisp, by contrast, has special variables and dynamic scoping. `with-output-to-string` in Common Lisp rebinds `*standard-output*` so that every output function that defaults to `*standard-output*` automatically writes to the string buffer. uLisp does not implement special variables or dynamic scoping. Therefore, `with-output-to-string` in uLisp can only capture output that is explicitly directed to the bound stream variable.

This is a deliberate simplification. Dynamic scoping and special variables add significant complexity to the evaluator and the environment representation. On a microcontroller where every byte of flash and every cycle of CPU matters, omitting them is a reasonable tradeoff. The user pays a small ergonomic cost—passing the stream argument explicitly—in exchange for a simpler, smaller interpreter.

---

## 7. Related Functions: PRINC-TO-STRING and PRIN1-TO-STRING

uLisp provides two convenience functions that use the same string-capture machinery without the special-form overhead:

- `(princ-to-string obj)` — prints `obj` without escape characters, returns the string.
- `(prin1-to-string obj)` — prints `obj` with escape characters, returns the string.

Their implementation is straightforward. Here is `fn_princtostring` at line 4390:

```c
object *fn_princtostring (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  object *obj = startstring();
  prin1object(arg, pstr);
  return obj;
}
```

And `fn_prin1tostring` at line 4395:

```c
object *fn_prin1tostring (object *args, object *env) {
  (void) env;
  object *arg = first(args);
  object *obj = startstring();
  printobject(arg, pstr);
  return obj;
}
```

The difference between `princ` and `prin1` is the `PRINTREADABLY` flag. `prin1object` respects this flag and prints strings with quotes and escape sequences; `printobject` (used by `prin1-to-string`) does the same. `princ-to-string` uses `prin1object` but with `PRINTREADABLY` cleared, so strings print without quotes and characters print as themselves. (In uLisp, the naming convention is slightly inverted from Common Lisp; check the flag state in the source if you need exact behavior.)

These functions are useful when you only need to convert a single object to a string. `with-output-to-string` is needed when you want to concatenate multiple outputs, format complex text, or capture output from functions that print as a side effect.

---

## 8. Concrete Examples on the PicoCalc

Here are practical examples you can type into the uLisp REPL on the PicoCalc.

### Example 1: Concatenating text

```lisp
(with-output-to-string (out)
  (princ "Temperature: " out)
  (princ 23 out)
  (princ " C" out))
```

Result: `"Temperature: 23 C"`

### Example 2: Building a formatted list

```lisp
(defun describe-list (lst)
  (with-output-to-string (out)
    (princ "(" out)
    (dolist (x lst)
      (princ x out)
      (princ " " out))
    (princ ")" out)))

(describe-list '(1 2 3))
```

Result: `"(1 2 3 )"` (note the trailing space, which you might want to trim).

### Example 3: Capturing error-like output

```lisp
(with-output-to-string (log)
  (princ "[INFO] Starting sensor read" log)
  (terpri log)
  (princ "[DATA] " log)
  (princ (analogread 26) log))
```

Result: `"[INFO] Starting sensor read\n[DATA] 512"` (or similar, depending on the analog reading).

### Example 4: What NOT to do

```lisp
(with-output-to-string (out)
  (print "This goes to the screen"))
```

Result on screen: `"This goes to the screen"`  
Return value: `""` (empty string)

Remember: always pass the stream variable to the output function.

---

## 9. Memory and GC Implications

Because `with-output-to-string` builds a linked-list string one character at a time, it allocates one or more objects from the workspace for every four characters (since each object cell holds four packed bytes). If you capture a long string—say, a 1,000-character log—you will allocate roughly 250 objects. On a Pico with 22,280 objects in the workspace, this is noticeable but not catastrophic.

However, if you are already low on memory, a large `with-output-to-string` can trigger garbage collection mid-body. This is why `protect(string)` is essential: without it, the half-built string could be swept away during a GC triggered by an unrelated allocation inside the body.

If you need to build very large strings repeatedly, consider whether you can stream the output directly to an SD card (`with-sd-card`) rather than buffering it in memory first.

---

## 10. Key Points to Remember

- `with-output-to-string` is a special form, not a function. It does not evaluate its first argument; it binds it as a variable.
- The bound variable is a `STRINGSTREAM` object. You must pass it explicitly to `print`, `princ`, `write-string`, and other output functions.
- There is no dynamic `*standard-output*` in uLisp. Output functions default to `pserial` (the screen/serial port) when no stream is provided.
- Characters are accumulated via the global variables `GlobalString` and `GlobalStringTail`, using the `pstr` callback.
- The string is protected from garbage collection during body evaluation.
- The return value is the accumulated string; the value of the last body form is discarded.
- For single-object conversion, `princ-to-string` and `prin1-to-string` are simpler and more direct.

---

## 11. References

| File | Line | Description |
|------|------|-------------|
| `ulisp-picocalc.ino` | 3103 | `sp_withoutputtostring` implementation |
| `ulisp-picocalc.ino` | 1620 | `startstring()` initializes the capture buffer |
| `ulisp-picocalc.ino` | 1715 | `pstr()` callback appends characters |
| `ulisp-picocalc.ino` | 1633 | `buildstring()` packs characters into linked-list cells |
| `ulisp-picocalc.ino` | 491 | `stream()` constructor for STREAM objects |
| `ulisp-picocalc.ino` | 2235 | `enum stream` defines `STRINGSTREAM` |
| `ulisp-picocalc.ino` | 2390 | `pfun_string()` returns `pstr` for string streams |
| `ulisp-picocalc.ino` | 2506 | `pstreamfun()` resolves stream argument to callback |
| `ulisp-picocalc.ino` | 2486 | `stream_table[]` dispatch table |
| `ulisp-picocalc.ino` | 4390 | `fn_princtostring` single-object conversion |
| `ulisp-picocalc.ino` | 4395 | `fn_prin1tostring` single-object conversion |
| `ulisp-picocalc.ino` | 5946 | Documentation string for `with-output-to-string` |
