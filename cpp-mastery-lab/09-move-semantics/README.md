# 09 — Move semantics

## The concept

Before C++11, "returning" or "passing" a large object meant copying it —
even when the source was a temporary about to be destroyed anyway. Move
semantics lets you *steal* a temporary's resources instead of copying them:
same end result, no copy cost.

### Lvalues, rvalues, and `T&&`

- An **lvalue** has a name/identity you can take the address of (`x`, `*p`,
  `v[0]`).
- An **rvalue** is a temporary with no persistent identity (`5`, `x + y`,
  the return value of a function by value).
- `T&&` (rvalue reference) binds only to rvalues — it's the type system's
  way of saying "this is a temporary, safe to cannibalize."

```cpp
class Buffer {
    int* data_;
    size_t size_;
public:
    // move constructor: STEAL other's pointer, leave other empty
    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;   // other must be left in a valid, destructible state
        other.size_ = 0;
    }
    // move assignment: same idea, but must also release our own resource first
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_; size_ = other.size_;
            other.data_ = nullptr; other.size_ = 0;
        }
        return *this;
    }
    ~Buffer() { delete[] data_; }   // safe even if data_ is nullptr
};
```

### `std::move`

`std::move(x)` doesn't move anything — it's just a cast to `T&&`, telling
the compiler "treat `x` as an rvalue from here on, it's OK to steal from
it." After `auto y = std::move(x);`, `x` is in a "valid but unspecified"
state — you may destroy or reassign it, but shouldn't read its value.

### Why `noexcept` matters here

`std::vector` only uses your move constructor during reallocation if it's
marked `noexcept` (or has no accessible copy constructor) — otherwise it
falls back to copying, because a throwing move mid-reallocation would leave
the vector in a state it can't safely roll back from. Always mark move
operations `noexcept` when they genuinely can't throw (true whenever you're
just swapping pointers, as above).

### Perfect forwarding (brief mention)

`template <typename T> void f(T&& x)` in a template context is a
*forwarding reference*, not an rvalue reference — it can bind to either
lvalues or rvalues, and `std::forward<T>(x)` preserves which one it was
when passing it on. This is how `std::make_unique`/`emplace_back` construct
objects in place without extra copies. Worth knowing exists; not the focus
of this module's exercises.

## Common traps

- Using a variable after `std::move`-ing it as if it still holds the
  original value.
- Move constructor/assignment that forgets to null out the source's
  pointer — the source's destructor then double-frees.
- Forgetting `noexcept`, silently losing the performance benefit inside
  `std::vector` growth.
- Self-move-assignment (`a = std::move(a);`) — the `this != &other` check
  in move assignment guards against this exact case.

## Run it

```bash
./09-move-semantics-demo
./09-move-semantics-exercises
```
