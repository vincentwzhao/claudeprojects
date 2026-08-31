# 08 — RAII (Resource Acquisition Is Initialization)

## The concept

RAII is C++'s core idiom for resource safety: tie a resource's lifetime to
an object's lifetime. Acquire the resource in the constructor, release it
in the destructor. Because C++ *guarantees* destructors run when an object
goes out of scope — including when an exception unwinds the stack — RAII
gives you automatic cleanup with no `try/finally` needed.

```cpp
class FileHandle {
    FILE* f_;
public:
    explicit FileHandle(const char* path) : f_(fopen(path, "r")) {}
    ~FileHandle() { if (f_) fclose(f_); }   // ALWAYS runs, even on exception
};

void process() {
    FileHandle f("data.txt");
    might_throw();          // if this throws, f's destructor STILL runs
}   // f closed here on normal return too
```

Compare to manual resource management, where every early return and every
exception path is a place you could forget to release the resource:

```cpp
FILE* f = fopen("data.txt", "r");
if (error_condition) return;      // LEAK: forgot to fclose
might_throw();                     // LEAK: exception skips the fclose below
fclose(f);
```

## Smart pointers are RAII for heap memory

- `std::unique_ptr<T>` — exclusive ownership, zero overhead over a raw
  pointer, not copyable (only movable — see `09-move-semantics`). Default
  choice for owning a heap object.
- `std::shared_ptr<T>` — shared ownership via reference counting; the
  object is destroyed when the last `shared_ptr` to it goes away. Has real
  overhead (atomic refcount) — use it only when ownership genuinely needs
  to be shared, not as a default.
- `std::weak_ptr<T>` — a non-owning observer of a `shared_ptr`-managed
  object; used to break reference cycles (two `shared_ptr`s pointing at
  each other never hit zero refcount without one being a `weak_ptr`).

```cpp
auto p = std::make_unique<Widget>(args);   // prefer make_unique over `new`
```

RAII isn't limited to memory: `std::lock_guard`/`std::unique_lock` RAII a
mutex lock (see `10-concurrency`), `std::fstream` RAII's a file handle,
`std::vector` RAII's its heap buffer.

## Common traps

- Manually calling `delete` on something already owned by a `unique_ptr`/
  `shared_ptr` — double free.
- Storing a raw pointer obtained from `unique_ptr::get()` past the smart
  pointer's lifetime — dangling.
- Reference cycles with `shared_ptr` (A holds `shared_ptr<B>`, B holds
  `shared_ptr<A>`) — neither refcount ever reaches zero; break with
  `weak_ptr` on one side.
- Throwing from a destructor — destructors run during stack unwinding from
  another exception, and an exception escaping a destructor during unwind
  calls `std::terminate`. Destructors should not throw.

## Run it

```bash
./08-raii-demo
./08-raii-exercises
```
