# 11 — Debugging

## The concept

This module is different from the others: instead of a concept demo +
exercises, you get `buggy.cpp` — six real, distinct memory bugs — and
`fixed.cpp`, the corrected versions. Your job is to find each bug in
`buggy.cpp` using real tools *before* comparing against `fixed.cpp`. This is
the actual skill the role wants: comfortable debugging from the terminal,
not just writing correct code the first time.

Each bug is its own numbered mode, selected by argv[1]:

```bash
./11-debugging-buggy 1   # use-after-free
./11-debugging-buggy 2   # double free
./11-debugging-buggy 3   # heap buffer overflow
./11-debugging-buggy 4   # memory leak
./11-debugging-buggy 5   # uninitialized read
./11-debugging-buggy 6   # stack buffer overflow
```

## Workflow: build with sanitizers first

AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) instrument
the binary to catch memory bugs *the instant they happen*, with a stack
trace — this is almost always faster than reasoning it out by eye or even
using gdb cold.

```bash
cd build   # or a fresh build dir
cmake -DSANITIZE=ON ..
make -j
./11-debugging/11-debugging-buggy 1
```

ASan's report tells you: what kind of bug (heap-use-after-free, etc.), the
exact line of the bad access, and — for use-after-free/double-free — the
line where the memory was originally allocated and freed. Read the report
top to bottom; the first frame in "READ/WRITE of size N at address ..." is
your bug's location.

## Workflow: valgrind (when you don't control the build, or want leak detail)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..   # ASan off — valgrind and ASan don't mix well
make -j
valgrind --leak-check=full --show-leak-kinds=all ./11-debugging/11-debugging-buggy 4
```
Valgrind is slower than ASan (runs everything through an emulator) but
needs no recompilation and gives very precise leak reports ("definitely
lost N bytes in 1 block, allocated at buggy.cpp:LINE").

## Workflow: gdb (when you need to inspect state interactively)

```bash
gdb ./11-debugging/11-debugging-buggy
(gdb) run 6                      # pass the bug number as a program argument
(gdb) bt                          # backtrace once it crashes — where were we?
(gdb) frame 2                     # jump to a specific frame in the backtrace
(gdb) print some_variable         # inspect a value
(gdb) list                        # show source around the current line
```

Useful during exploration even before a crash:
```bash
(gdb) break buggy.cpp:42          # set a breakpoint at a line
(gdb) watch some_variable         # break whenever some_variable changes
(gdb) next / step                 # step over / step into
(gdb) continue                    # resume until next breakpoint/crash
```

For a use-after-free specifically, `gdb` alone won't tell you *where* the
memory was freed — that's ASan's/valgrind's specialty. Use gdb to inspect
live state and control flow; use ASan/valgrind to pinpoint memory-lifetime
bugs. Knowing which tool answers which question is itself the skill being
tested here.

## The six bugs (don't peek until you've tried)

1. **Use-after-free** — reading/writing through a pointer after its memory
   was `delete`d.
2. **Double free** — calling `delete`/`free` twice on the same pointer.
3. **Heap buffer overflow** — writing past the end of a heap allocation.
4. **Memory leak** — losing the only pointer to allocated memory without
   freeing it.
5. **Uninitialized read** — reading a variable before it's ever assigned.
6. **Stack buffer overflow** — writing past the end of a fixed-size local
   array.

## Run it

```bash
./11-debugging-fixed     # runs all six FIXED versions; exits 0, all correct
./11-debugging-buggy N   # N = 1..6, explore each bug with the tools above
```
