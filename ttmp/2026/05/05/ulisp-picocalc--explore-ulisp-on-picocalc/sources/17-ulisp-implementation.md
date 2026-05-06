## Implementation

The main challenge of writing uLisp was enabling it to run on a processor like the ATmega328. Although this has 32 Kbytes of flash program memory, it only provides 2 Kbytes of RAM.

Most Lisp systems store their entire workspace in RAM, including symbols, system functions, user-defined functions, and data. Clearly this wasn't going to be possible in 2 Kbytes. The solution was to store as much as possible in the program memory, and only use RAM when absolutely necessary.

After experimenting with several alternatives I came up with the following solution:

- The names of the built-in system functions are stored in a lookup table in program memory.
- User-defined symbols are restricted to three letters or digits, so they can be packed into a single 16-bit word (uLisp 1.8 removed this restriction).
- The arithmetic uses signed integers, so they will fit into a single word.
- Every Lisp object is a two-word cell, representing either a symbol, number, or cons.
- Garbage collection marks objects by setting the bottom bit of the leftmost word. This is unused because the workspace is aligned to an even number of bytes.

### Acknowledgements

In the early stages of designing the interpreter I found Nurullah Akkaya's article "A micro-manual for LISP Implemented in C" very useful <sup><a href="http://www.ulisp.com/show?1AWG#cite_note1">[1]</a></sup>.

For the garbage collector I found Bob Nystrom's article "Baby's First Garbage Collector" useful <sup><a href="http://www.ulisp.com/show?1AWG#cite_note2">[2]</a></sup>.

For help with making the interpreter tail-recursive I relied on the section "A Properly Tail-Recursive Interpreter" in Peter Norvig's classic book "Paradigms of Artificial Intelligence Programming" <sup><a href="http://www.ulisp.com/show?1AWG#cite_note3">[3]</a></sup>.

I am also grateful to the user **clawson** on AVR Freaks who showed me how to design a procedure name lookup table in program memory.

---

1. [^](http://www.ulisp.com/show?1AWG#cite_ref1) [A micro-manual for LISP Implemented in C](http://nakkaya.com/2010/08/24/a-micro-manual-for-lisp-implemented-in-c/) on nakkaya dot com.
2. [^](http://www.ulisp.com/show?1AWG#cite_ref2) [Baby's First Garbage Collector](http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/) on stuffwithstuff.
3. [^](http://www.ulisp.com/show?1AWG#cite_ref3) Norvig, Peter "Paradigms of Artificial Intelligence Programming" Morgan Kaufmann Publishers, Inc, San Francisco, 1992, pp 766-768.

---

Previous: [Adding useful functions to uLisp](http://www.ulisp.com/show?3GSE)

Next: [Objects](http://www.ulisp.com/show?1BLW)