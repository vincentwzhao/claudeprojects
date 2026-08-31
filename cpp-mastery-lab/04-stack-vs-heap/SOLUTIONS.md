# 04 — Stack vs. heap: solutions

```cpp
int* make_answer() {
    int* p = new int(42);   // heap: survives past this function's return
    return p;
}

int sum_stack(int n) {
    int buf[16];             // fixed-size, on the stack, no allocation call
    int total = 0;
    for (int i = 0; i < n; ++i) {
        buf[i] = i;
        total += buf[i];
    }
    return total;
}

long sum_heap(int n) {
    int* buf = new int[n];   // size only known at runtime -> must be heap
    long total = 0;
    for (int i = 0; i < n; ++i) {
        buf[i] = i;
        total += buf[i];
    }
    delete[] buf;
    return total;
}
```

Notes:
- `make_answer`'s original bug (returning `&local`) is the single most
  common "why does this sometimes print the right value and sometimes
  garbage" question in interviews — the stack slot is reused by the very
  next function call, so the answer being briefly "correct" is coincidence,
  not correctness.
- `sum_stack` only works because the test caps `n` at 16 and `buf` is sized
  16. That's the actual constraint of stack allocation — you must know the
  size at compile time (or use a VLA, a non-standard GCC/Clang extension —
  don't rely on it). `sum_heap` exists precisely because heap allocation
  removes that constraint at the cost of speed and manual lifetime
  management.
