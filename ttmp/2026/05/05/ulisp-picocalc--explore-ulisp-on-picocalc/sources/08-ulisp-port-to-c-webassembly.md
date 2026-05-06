[**eliot**](http://forum.ulisp.com/u/eliot) #1

Hello fellow Lispers,

I’m pleased to share some progress made on porting uLisp to C and WebAssembly.

- Demo: [https://eliot-akira.github.io/ulisp-wasm](https://eliot-akira.github.io/ulisp-wasm)
- Source code: [https://github.com/eliot-akira/ulisp-wasm](https://github.com/eliot-akira/ulisp-wasm)

---

![30](http://forum.ulisp.com/uploads/default/original/1X/f141160ca6ad79f0acfad270fb22258a87e83d8d.gif)

The C port is adapted from the [ESP32 version](https://github.com/technoblogy/ulisp-esp/) written for Arduino (a variant/subset of C++).

For the conversion, I studied this prior art posted here:

Based on this example I gradually rewrote or patched the Arduino/ESP-related parts of `ulisp-esp` (~8000 lines) into generic C99. It’s compiled with CMake and [Clang](https://clang.llvm.org/).

It can now evaluate Lisp expressions but missing most features like I/O devices. So far I’ve run it on desktop Linux (Ubuntu) and Raspberry Pi Zero (32-bit Raspbian). A start of a tiny **command-line interface** (CLI) and **interactive shell** (REPL) in the terminal.

---

![image](http://forum.ulisp.com/uploads/default/original/1X/832c0b5ed9750b9288b9b6f9381eae73782c81df.png)

The **WebAssembly binary** is built from the same C code using [Emscripten](https://emscripten.org/).

It runs in the browser, and on server-side JavaScript runtimes like V8, Node/Bun/Deno. A few functions were added to the uLisp interpreter to communicate to/from the host, to send messages or emulate ports and devices. This build target has potential to extend uLisp with features provided by the host, like network access (HTTP, WebSocket), servers, file system, database.

A much smaller bare Wasm binary can be built using Clang, that runs on Wasm runtimes with [WebAssembly System Interface (WASI)](https://nodejs.org/api/wasi.html#webassembly-system-interface-wasi) - a kind of abstract interface for POSIX-like I/O. I’m still learning how to make it work.

---

The [test suite](https://github.com/technoblogy/ulisp-builder/tree/main/Test%20Suites) from `ulisp-builder` was ported to the project, and eventually I got `ulisp-wasm` to pass all 640 tests.

For the code editor on the web, I used [CodeMirror](https://codemirror.net/) with language extension for Lisp syntax highlight and [Paredit](https://calva.io/paredit/) for basic structural editing (experimental).

It’s all still a rough prototype, made using tools I’m comfortable with, like TypeScript and Docker. I plan to extract the core into a small standalone C library - but in its current state it’s probably not usable by anyone yet. It’s been fun, technically challenging, and I’m learning a lot.

Next steps and ideas:

- Playground with editor and live preview
- Documentation with runnable code examples
- Import/export, shareable links
- Emulate ports and devices - LED lights, display/canvas, local storage
- Single-file executable binary for major operating systems: Linux, macOS, Windows
- uLisp-based OS with Wasm [unikernel](http://unikernel.org/) on bare metal:¬)

---

One of the problems with the Web Assembly virtual machine is lack of support for deep recursion.  
I think the parser needs to turn a tail recursive form into an imperative form.

It would be very natural in Lisp to write a recursive main “loop” that cycles forever.

```
Error: Maximum steps exceeded: 999999
```

---

uLisp already turns tail recursive forms into imperative forms. You can see this by printing **(locals)**, which is the Lisp stack.

For example:

```
(defun tail-recursive (n)
  (print n)
  (print (locals))
  (delay 500)
  (tail-recursive (incf n)))

> (tail-recursive 1)

1 
((n . 1)) 
2 
(nil (n . 2) nil (n . 2)) 
3 
(nil (n . 3) nil (n . 2)) 
4 
(nil (n . 4) nil (n . 2)) 
5 
(nil (n . 5) nil (n . 2))
```

In this case the stack doesn’t grow.

If you prevent the function from being tail recursive:

```
(defun not-tail-recursive (n)
  (print n)
  (print (locals))
  (delay 500)
  (+ (not-tail-recursive (incf n)) 2)

> (not-tail-recursive 2)

2 
((n . 2)) 
3 
((n . 3) (n . 3)) 
4 
((n . 4) (n . 4) (n . 3)) 
5 
((n . 5) (n . 5) (n . 4) (n . 3)) 
6 
((n . 6) (n . 6) (n . 5) (n . 4) (n . 3))
```

In this case the stack grows continuously.

---

I’ll have to try once more with his online REPL (link in the post).  
I may have made a typo but I don’t know how to enter Ctrl on this phone so his REPL accepts it

I will say I used 1+ in tail position with 500,000 loops not incf (no need to mutate).

I’ll try again once I get home on a computer. Could be my error.

EDIT:

Hmm the following does indeed still produce an error in his REPL.

```
(defun test (n limit)
  (if (>= n limit)
    (print n)
    (test (1+ n) limit)))

(test 0 500000)
```

#### Console

```
Error: Maximum steps exceeded: 999999
```

---

It may be nothing to do with the recursion stack. The error message implies that the test is for iterations rather than stack size. Have you tried:

```
(dotimes (n 1000000) (print n))
```

---

```
...
249995 
249996 Error: Maximum steps exceeded: 999999
```

---

[**johnsondavies**](http://forum.ulisp.com/u/johnsondavies) 2025-08-29 13:08:04 UTC #7

OK, so it’s nothing to do with recursion. There’s something either in WebAssembly, or in the port of uLisp by [@eliot](http://forum.ulisp.com/u/eliot), limiting the number of statements it can execute.

---

[**hasn0life**](http://forum.ulisp.com/u/hasn0life) 2025-08-29 14:38:42 UTC #8

Is the `locals` function documented anywhere? Cause that’s really useful

---

It’s in the built-in documentation:

```
> (? locals)
(locals)
Returns an association list of local variables and their values.
```

I should probably add it to the Language Reference.

---

> Error: Maximum steps exceeded: 999999

To clarify, this is an optional and customizable limit added to the host. It’s meant to prevent locking up the editor interface if the execution takes too long in terms of “steps”.

It’s actually a temporary measure, as I’m still working on running the uLisp interpreter in a [worker](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API) thread separate from the main execution thread (JavaScript and UI). Then it will be able to loop indefinitely in parallel, without interfering with the editor.

---

> Web Assembly virtual machine is lack of support for deep recursion. I think the parser needs to turn a tail recursive form into an imperative form

Good point, Wasm did indeed lack tail-call optimization until recently. Apparently it’s now supported by major browsers. [https://webassembly.org/features/#:~:text=Tail,Call](https://webassembly.org/features/#:~:text=Tail,Call)

But that’s for tail recursion in the Wasm bytecode as a compile target. There’s an example given in [this article](https://v8.dev/blog/wasm-tail-call) when V8 (JS/Wasm engine) added support for it. So if the uLisp interpreter used tail recursion in its C source code (and the tail was explicitly marked as such using an attribute `__musttail__`) then that would be compiled by Emscripten to use the new tail call instructions in Wasm.

When running Lisp code, the uLisp interpreter handles tail calls - if I’m not mistaken, [here](https://github.com/technoblogy/ulisp-esp/blob/ebc4a1172946105b6bd92da1ae7652896dc255ef/ulisp-esp-comments.ino#L7750-L7756) in the `eval` function.

```
while (form != NULL){
  object *obj = cons(eval(car(form),env),NULL);
  cdr(tail) = obj;
  tail = obj;
  form = cdr(form);
  nargs++;
}
```

I love this part of Lisp interpreters in general, it’s elegant how recursion is unrolled into a single `while` loop.

(Edit: Found an article on uLisp site that explains how it’s implemented: [Tail-call optimization](http://www.ulisp.com/show?1BBP). It’s not the above code section, it’s several places with `goto EVAL`. So the code is not necessarily pretty to look at, but it’s still conceptually beautiful.:)

---

> limiting the number of statements it can execute

For this I added a function `yield_loop()` to the interpreter to call the host on every “step”. A step is each part of a statement, including function arguments - I guess it’s more properly termed a “node” of an expression in the abstract syntax tree.

The host can decide when to advance to the next step, effectively pausing the runtime; or to stop execution for any reason, like timeout or user action. (Aha, so I can add a play/pause/stop button in the web interface, when it supports infinite loop.)

This feature to run the code step by step, I’m hoping to also use it for educational purpose - for example in a tutorial, visually highlighting parts of the code as it runs.

---

I did find a way to cause an actual **stack overflow** error in the web interface though, simply by running `(fib 511)`.

That shouldn’t have happened, since the tail call is supposed to be eliminated to not use the stack. And the loop should have been stopped by the maximum steps limit instead.

So that’s something to solve. I’ll need to find out if the error is thrown from JS or uLisp side. …Oh, since the error message is in lowercase, `stack overflow`, I found it [here](https://github.com/technoblogy/ulisp-esp/blob/ebc4a1172946105b6bd92da1ae7652896dc255ef/ulisp-esp-comments.ino#L7652) in the interpreter.

```
(defun fib (n)
  (if (< n 3) 1
    (+ (fib (- n 1)) (fib (- n 2)))))

(fib 511)
```

When it’s `(fib 51)` it stops due to max steps limit. When it’s `(fib 511)` it causes stack overflow.

Maybe because `fib` is calling itself twice in the final statement, and the first one is not considered part of the “tail”?

---

Ah, I figured it out. I found an example of a tail-recursive Fibonacci function in Scheme, and converted it to uLisp.

```
(defun fib (n)
  (fib-tr n 1 0))

(defun fib-tr (n next result)
  (if (= n 0) result
    (fib-tr (- n 1) (+ next result) next)))

(fib 51)
```

Running `(fib 51)` returns `20365000000`. And `(fib 511)` returns `Inf`.

So I think tail-call elimination is working.

---

[**Pixel\_Outlaw**](http://forum.ulisp.com/u/Pixel_Outlaw) 2025-08-30 01:15:10 UTC #12

Seems like you’re on you way then!  
Yeah, that version with two fib’s in the same s-expression is *doubly* tail recursive which is undesirable.  
When there is “nothing left to compute” but the next recursion THEN it’s tail call optimizable.  
This is why when you do addition with (+ (foo …)) it can’t complete until all foo functions are run to be summed.

There is an extension to the usual algorithm ( tail recursion modulo cons) to handle a few special cases in exception to the rule.  
But generally most implementations want the ONLY thing left to be calculated is the next recursion.

Here is a link to the extension:  

---

Thank you for the article about “tail-recursion modulo cons” transformation. That’s all new to me, I didn’t know there were algorithms for automatically transforming “not quite tail-recursive” functions. I’ll enjoy reading through it.

---

Based on the feedback I updated the web demo with new features.

[![image](http://forum.ulisp.com/uploads/default/original/1X/e675e6d98e5898f5a06d93bd5259272fa68fd7e1.png)](http://forum.ulisp.com/uploads/default/original/1X/e675e6d98e5898f5a06d93bd5259272fa68fd7e1.png "image.png")

- Run uLisp in its own worker thread. Remove max steps limit, letting the code run indefinitely.
- Stop button to terminate worker from host.
- Steps counter to show when code is running.
- Share-able links with the source code base64-encoded in the URL hash.

---

The above code example looks like this when encoded as a link.

- [https://eliot-akira.github.io/ulisp-wasm/#H4sIAAAAAAAAA6tWSs5PSVWyUtJISU0rzVMoSczM0S1KTS4tKs4sS1XQyNOMyVNQ0CgoyswrUUDhaOTkJyfmFGtCxFJScxIrFUwNDCBcdGMy85LTgNpBimPy0GUNgaJKtQCHleZligAAAA~~](https://eliot-akira.github.io/ulisp-wasm/#H4sIAAAAAAAAA6tWSs5PSVWyUtJISU0rzVMoSczM0S1KTS4tKs4sS1XQyNOMyVNQ0CgoyswrUUDhaOTkJyfmFGtCxFJScxIrFUwNDCBcdGMy85LTgNpBimPy0GUNgaJKtQCHleZligAAAA~~)

Theoretically it should run forever until stopped.

The example also shows the `delay` function working. It uses a neat feature in Emscripten that lets the Wasm side (C and uLisp code) call an async JavaScript function provided by the host, pausing the runtime until the response comes back - in this case, after `setTimeout()` is done. On the Wasm side it behaves like a synchronous function.

- [Making async Web APIs behave as if they were synchronous](https://emscripten.org/docs/porting/asyncify.html#making-async-web-apis-behave-as-if-they-were-synchronous)

---

Trying a few examples from before… (Each title is a link to the code.)

##### Not-tail-recursive

The stack keeps growing continuously.

```
(defun not-tail-recursive (n)
  (print n)
  (print (locals))
  (delay 500)
  (+ (not-tail-recursive (incf n)) 2)

(not-tail-recursive 2)
```

##### Not quite tail-recursive Fib

The interpreter throws a stack overflow error, as expected.

```
(defun fib (n)
  (if (< n 3) 1
    (+ (fib (- n 1)) (fib (- n 2)))))

(fib 9999)
```

##### Tail-recursive Fibonacci

```
(defun fib (n)
  (fib-tr n 1 0))

(defun fib-tr (n next result)
  (if (= n 0) result
    (fib-tr (- n 1) (+ next result) next)))

(fib 9999)
```

Keeps running until the number reaches `Inf` (infinity) using [`isinf`](https://en.cppreference.com/w/c/numeric/math/isinf) in C.

---

[**Pixel\_Outlaw**](http://forum.ulisp.com/u/Pixel_Outlaw) 2025-08-30 22:08:20 UTC #14

Nice, now I can poke code in from my phone!

---

[**nanomonkey**](http://forum.ulisp.com/u/nanomonkey) 2025-09-12 20:07:22 UTC #15

In another thread [@eliot](http://forum.ulisp.com/u/eliot) mentioned " With my interest in WebAssembly, which has a fairly small [set of instructions](https://webassembly.github.io/spec/core/text/instructions.html), it makes me dream of a Lisp to Wasm compiler." But it seems more appropriate to respond in this thread…

Have you checked out [Hoot](https://www.spritely.institute/hoot/) which is a Guile Scheme to WASM compiler. The interpreter works in the browser, and the compiler is run off of Guile scheme. Quite nice as it utilizes the more recent WasmGC for garbage collection instead of compiling one from scratch.

---

[**eliot**](http://forum.ulisp.com/u/eliot) 2025-09-14 17:45:04 UTC #16

Fascinating project, Guile Hoot. I’ve read a few posts about it before, and it’s pretty much what I dream of in a **Lisp to Wasm compiler**.

I had a local Git repo of Hoot from the last time I looked into it. I hadn’t seen any commits recently but from the linked page I learned the repo was moved from Gitlab to Codeberg! I guess there’s no mechanism in Git to let downstream/forks know that. I updated the remote URL and pulled the newest commits. Curious to read through it and learn what they’re working on. Following the [instructions to build from source](https://codeberg.org/spritely/hoot#building-from-source) results in a mysterious error though, so I think the edge of development is a moving target. When I did try the Wasm compiler, I was surprised how fast and compact the compiled binary file was.

- [Directly compiling Scheme to WebAssembly: lambdas, recursion, iteration!](https://www.spritely.institute/news/scheme-to-wasm-lambdas-recursion.html)
- [A Scheme Primer](https://files.spritely.institute/papers/scheme-primer.html)

The concept is brilliant, the project is aiming for a vision that I feel is a promising direction with potential.

Guile Scheme leads to a whole world of Lispy goodness that I’ve only started exploring. A related project Guix even has [an OS distribution](https://guix.gnu.org/manual/en/html_node/GNU-Distribution.html), where one of the goals is to bootstrap the entire system from a tiny binary seed and “a small Scheme interpreter and a C compiler written in Scheme”.

- [Guix: The Reduced Binary Seed Bootstrap](https://guix.gnu.org/manual/en/html_node/Reduced-Binary-Seed-Bootstrap.html)

I suppose it’s possible to write a uLisp to Guile Scheme translator, which would benefit from the latter’s language ecosystem including the Wasm compiler.

---

![image](http://forum.ulisp.com/uploads/default/original/1X/b8de536670b16a28ae2d6f07bcdb8f294439439c.png)

On an unrelated note, a little progress update: I’ve been able to run `ulisp-c` in a native app made with [Sokol](https://floooh.github.io/sokol-html5/), a cross-platform GUI library (like a game engine with graphics, audio, timer, user input). It compiles to a single-file executable (1.3 MB so far), and supports building for Linux/Mac/Windows and WebAssembly.