# 01 — Pointers

## The concept

A pointer is just a variable whose value is a memory address. That's it — but
almost every C++ bug (segfaults, use-after-free, buffer overruns, dangling
references) traces back to someone forgetting that fact.

Key mechanics to internalize:

- `T* p` declares a pointer to `T`. `*p` dereferences it (follow the address
  to get the value). `&x` takes the address of `x`.
- `nullptr` is "points at nothing" — dereferencing it is undefined behavior.
- Pointer arithmetic (`p + 1`) moves by `sizeof(T)` bytes, not 1 byte — this
  is why array indexing (`a[i]` is sugar for `*(a + i)`) works.
- A pointer doesn't own anything by default. Nothing stops you from having
  two pointers to the same memory, or a pointer to memory that's already
  been freed (a **dangling pointer**).
- `const` and pointers combine three ways: `const T* p` (can't modify what's
  pointed to), `T* const p` (can't repoint), `const T* const p` (neither).
  Read pointer declarations right-to-left: `const T* p` = "p is a pointer to
  a const T".
- `void*` is a typeless pointer — you lose the ability to dereference or do
  arithmetic on it until you cast it back.
- Function pointers (`int (*f)(int)`) let you pass behavior as data — the
  ancestor of `std::function` and lambdas.

## Why interviewers care

Pointers are the mechanism *and* the trap. Expect: "what's the difference
between a pointer and a reference," "what happens if you dereference a null
pointer," "what's a dangling pointer, how do you get one, how do you avoid
it," and live coding that requires manual pointer manipulation (reverse a
linked list, swap via pointers, implement `strlen`).

## Common traps

- Dereferencing an uninitialized or null pointer.
- Returning the address of a local (stack) variable from a function — the
  stack frame is gone the instant the function returns.
- Pointer arithmetic past the end of an array (off-by-one UB).
- Comparing pointers from unrelated arrays.
- Forgetting that `sizeof(pointer)` is the pointer's size (8 bytes on 64-bit),
  *not* the size of what it points to.

## Run it

```bash
./01-pointers-demo
./01-pointers-exercises
```

`demo.cpp`'s last section (`the_dangling_trap`) deliberately dereferences a
dangling pointer — real undefined behavior. On this machine it segfaults;
on another compiler/platform it might print a plausible-looking value
instead. Both outcomes are "correct" for UB, which is exactly the point:
you cannot rely on a crash to tell you a dangling-pointer bug exists.

