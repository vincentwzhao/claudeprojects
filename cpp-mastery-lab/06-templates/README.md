# 06 — Templates

## The concept

Templates are C++'s compile-time generics: write code once, parameterized
over a type (or value), and the compiler generates a concrete version for
every type you actually instantiate it with. This is how the entire STL
(`std::vector<T>`, `std::sort<Iter>`, ...) works — templates aren't a niche
feature, they're the foundation of `07-stl`.

### Function templates

```cpp
template <typename T>
T max_of(T a, T b) { return a > b ? a : b; }

max_of(3, 5);        // T deduced as int
max_of(3.0, 5.0);     // T deduced as double
```
The compiler deduces `T` from the arguments at the call site — no manual
`<int>` needed unless deduction is ambiguous.

### Class templates

```cpp
template <typename T>
class Box {
    T value_;
public:
    explicit Box(T v) : value_(v) {}
    T& get() { return value_; }
};

Box<int> b(5);
```

### Non-type template parameters

```cpp
template <typename T, int N>
struct Array { T data[N]; };

Array<int, 10> a;   // N is a compile-time constant, part of the type
```

### Why templates and not just runtime polymorphism (virtual functions)?

Templates specialize at **compile time** — no vtable indirection, no
runtime dispatch cost, and the compiler can inline/optimize aggressively
because it knows the concrete type. The cost: every instantiation is
separately compiled code (binary size), and template error messages can be
notoriously long. Virtual functions specialize at **runtime** — one copy of
code, dispatch through a vtable, works when the concrete type isn't known
until runtime (e.g. a heterogeneous container of `Shape*`).

### Compile-time facilities worth knowing exist

- `static_assert(condition, "message")` — compile-time assertion.
- `if constexpr` (C++17) — compile-time branch; the untaken branch doesn't
  even need to compile for the instantiated type.
- Concepts (C++20) — constrain what types a template accepts, with
  readable error messages instead of a wall of substitution failures.
  This module sticks to pre-concepts style first since it's what you'll
  read in most existing/interview codebases, but a demo is included.

## Common traps

- Template code lives in headers (or is `#include`d as if it does) because
  the compiler needs the full definition at every instantiation point —
  forgetting this gives you baffling linker errors ("undefined reference")
  for template code split into a `.cpp`.
- Assuming templates behave like a single compiled function — they don't;
  each distinct set of template arguments is its own compiled entity
  ("template instantiation"), which is why heavy template use can bloat
  binaries.
- Confusing "the compiler will find *a* valid instantiation" with "the
  compiler will find the *intended* one" — implicit conversions during
  argument deduction can silently pick a type you didn't mean.

## Run it

```bash
./06-templates-demo
./06-templates-exercises
```
