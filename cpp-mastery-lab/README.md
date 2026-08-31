# cpp-mastery-lab

A hands-on C++ curriculum built around the ⭐⭐⭐⭐⭐ core of a systems/infra-style
interview prep stack: **C/C++** and the mental model it forces on you (pointers,
memory, ownership) that everything else — data structures & algorithms, Linux,
computer architecture, OS internals — sits on top of.

This repo doesn't try to teach DSA, Linux, or OS as separate subjects. It teaches
the eleven C++ mechanisms below *deeply*, because they're the vocabulary those
subjects are written in: you can't reason about cache lines without pointers,
can't reason about a segfault without stack vs. heap, can't reason about a
deadlock without concurrency primitives.

## How this is organized

Eleven topics, each its own folder, each with the same three pieces:

| File | Purpose |
|---|---|
| `README.md` | The concept, why it matters, the interview traps |
| `demo.cpp` | A working, heavily-commented program you run to *see* the concept |
| `exercises.cpp` | TODO stubs you fill in; self-checks via `CHECK()`/`CHECK_EQ()` |
| `SOLUTIONS.md` | Reference answers — don't peek until you've tried |

```
01-pointers/
02-references/
03-memory-allocation/
04-stack-vs-heap/
05-structs-and-classes/
06-templates/
07-stl/
08-raii/
09-move-semantics/
10-concurrency/
11-debugging/
```

Work through them roughly in order — each one leans on the last (references
need pointers, RAII needs stack-vs-heap, move semantics needs RAII, concurrency
needs all of it).

## Build & run

```bash
mkdir build && cd build
cmake ..
make -j
ctest --output-on-failure     # runs every exercises.cpp and reports pass/fail

# run a single demo or exercise set directly, e.g.:
./01-pointers/01-pointers-demo
./01-pointers/01-pointers-exercises
```

### Debugging with real tools

```bash
cmake -DSANITIZE=ON ..        # rebuild with ASan + UBSan instrumentation
make -j
./11-debugging/11-debugging-buggy   # watch it explode with a real diagnosis

gdb ./11-debugging/11-debugging-buggy
valgrind --leak-check=full ./11-debugging/11-debugging-buggy
```

`11-debugging/README.md` walks through gdb and valgrind commands specifically.

## Where this fits in the bigger prep plan

This repo is the C++ leg of a broader stool. Once these eleven topics feel
natural, the highest-leverage next steps (outside this repo) are:

1. **Data structures & algorithms** — reimplement the classics (hash map,
   BST, heap, graph traversal) *in this repo's style*: raw pointers first,
   then STL. Use `06-templates` + `07-stl` as the bridge.
2. **Linux + systems** — `strace`, `perf`, `/proc`, file descriptors, signals.
   `11-debugging` and `10-concurrency` are the on-ramp.
3. **Computer architecture** — cache lines, false sharing, branch prediction.
   `04-stack-vs-heap` and `03-memory-allocation` set up the vocabulary;
   profile the demos with `perf stat`/`perf record` once you're comfortable.
4. **Operating systems** — processes vs. threads, virtual memory, scheduling.
   `10-concurrency` is the direct link.
5. **Python** for automation/ML scripting, **ML fundamentals** (tensors,
   training/inference), **compilers** (LLVM/MLIR basics), **distributed
   systems**, and **AWS** — separate tracks, lower priority, pursued after
   the above feels solid.

The goal of this repo isn't to cover all of that — it's to make pointers,
ownership, and memory second nature so that everything downstream is easier
to reason about.
