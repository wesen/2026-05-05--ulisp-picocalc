## Tail-call optimization

uLisp has been designed to include tail-call optimization. This means that where the final action in a function is a call to another function, that tail call is replaced by a goto, ensuring that space is not wasted on the stack for storing the return address. Tail-call optimization is especially important in applications written using recursive functions, because it means that they can be as efficient as using iteration.

For example, a simple implementation of the standard Arduino blink function could be written:

```
(defun blink (&optional x) 
  (pinmode :led-builtin t)
  (digitalwrite :led-builtin x)
  (delay 1000)
  (blink (not x)))
```

Running it, by evaluating:

```
> (blink)
```

causes the built-in LED to flash indefinitely. Without tail-recursion each flash would allocate more space on the stack for the return address of **blink**, and eventually the stack would run out. With tail-recursion this version is just as efficient as a version using **loop**.

### How tail-recursion is implemented

There is a separate set of special forms, called tail forms, which return their result unevaluated. These have names starting with "tf\_". For example, the definition of **if** is:

```
object *tf_if (object *args, object *env) {
  if (eval(first(args), env) != nil) return second(args);
  return third(args);
}
```

This function is called in **eval()** as follows (a bit simplified):

```
object *eval (object *form, object *env) {
  EVAL:

  ...

  // It's a list
  object *function = car(form);
  object *args = cdr(form);

  // List starts with a symbol?
  if (function->type == SYMBOL) {
    unsigned int name = function->name;
    
    if (name > SPECIAL_FORMS && name < TAIL_FORMS) {
      return ((fn_ptr_type)lookupfn(name))(args, env);
    }

    if (name > TAIL_FORMS && name < FUNCTIONS) {
      form = ((fn_ptr_type)lookupfn(name))(args, env);
      goto EVAL;
    }
```

If it's a standard special form the C function is called by **lookupfn** and the value is returned.

If it's a tail-recursive function, like **tf\_if()**, the C function is called by **lookupfn**, the value is assigned to **form**, and a goto jumps back to the start of **eval()** to evaluate the new value without an additional function call.

The following uLisp special forms support tail-call optimization:

**progn**, **let**, **let\***, **return**, **if**, **cond**, **when**, **case**, **and**, and **unless**.

---

Previous: [Streams](http://www.ulisp.com/show?5837)

Next: [Saving and loading images](http://www.ulisp.com/show?1DTL)