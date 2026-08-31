# 05 — Structs & classes

## The concept

`struct` and `class` are the *same* mechanism in C++ — the only difference
is the default access specifier (`public` for struct, `private` for class)
and default inheritance mode. Convention: `struct` for passive data bags,
`class` for types with invariants to protect.

### The "special member functions" — the actual interview core

Every class has up to six compiler-generated member functions unless you
suppress or define them:

1. Default constructor
2. Destructor
3. Copy constructor `T(const T&)`
4. Copy assignment `T& operator=(const T&)`
5. Move constructor `T(T&&)` (see `09-move-semantics`)
6. Move assignment `T& operator=(T&&)`

The **Rule of Zero / Three / Five**:
- **Rule of zero**: if your class doesn't manage a raw resource directly
  (it only holds `std::string`, `std::vector`, `std::unique_ptr`, etc.),
  define *none* of the six — let the compiler generate correct ones from
  the members, which already manage themselves.
- **Rule of three**: if you need to write *any one* of destructor / copy
  constructor / copy assignment, you almost certainly need all three (e.g.
  you're managing a raw pointer manually).
- **Rule of five**: same as three, but also define move constructor/move
  assignment, or the compiler-generated copies will be used for moves too
  (silently slower, and if you disabled copy, moves become unavailable).

### Member initializer lists

```cpp
struct Point {
    int x, y;
    Point(int x_, int y_) : x(x_), y(y_) {}   // initializer list — preferred
};
```
Members are initialized in **declaration order** (not initializer-list
order — a classic `-Wreorder` warning), and initializer-list init happens
*before* the constructor body runs. For `const` members and reference
members, the initializer list is the *only* way to initialize them — you
can't assign to them in the body.

### Access control & encapsulation

`private` members plus a curated `public` interface is how you protect a
class's invariants (e.g. "size never exceeds capacity") from code outside
the class breaking them directly.

### Inheritance & virtual dispatch (brief)

```cpp
struct Shape {
    virtual double area() const = 0;   // pure virtual -> abstract base
    virtual ~Shape() = default;         // virtual destructor: mandatory
};                                        // whenever you delete via base ptr
struct Circle : Shape {
    double r;
    double area() const override { return 3.14159 * r * r; }
};
```
Forgetting `virtual` on the destructor of a base class you intend to delete
polymorphically (`Shape* s = new Circle(...); delete s;`) is undefined
behavior — only `Shape`'s destructor runs, `Circle`'s members leak.

## Common traps

- Forgetting a virtual destructor on a polymorphic base.
- Slicing: `Shape s = Circle(...);` copies only the `Shape` part — the
  circle-ness is gone. Pass/store polymorphic types by pointer or reference.
- Member initializer order bugs (initializing `y` using `x` when `x` is
  declared *after* `y`).
- Relying on the implicitly generated copy constructor for a class that
  holds a raw owning pointer (shallow copy -> double free).

## Run it

```bash
./05-structs-and-classes-demo
./05-structs-and-classes-exercises
```
