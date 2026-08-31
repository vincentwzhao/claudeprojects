# 02 — References

## The concept

A reference is an alias for an existing object — a second name for the same
memory, not a new object and not (conceptually) a pointer you can reseat or
null out. Under the hood compilers usually implement references as pointers,
but the *language rules* are different and that's what matters:

- Must be initialized when declared: `int& r = x;` — no `int& r;`.
- Cannot be reseated: once bound to `x`, `r = y;` assigns `y`'s value *into
  x*, it does not make `r` refer to `y`.
- Cannot be null (in well-defined code) — there's no reference equivalent of
  `nullptr`. (You *can* technically dereference a null pointer into a
  reference and get UB, but that's a bug, not a feature.)
- `const T&` can bind to a temporary, extending its lifetime to the
  reference's scope — this is why "pass by const reference" is the default
  way to accept read-only arguments without copying.

## Reference vs. pointer — the interview answer

| | Pointer | Reference |
|---|---|---|
| Can be null | yes | no (by contract) |
| Can be reseated | yes | no |
| Needs explicit deref | yes (`*p`) | no (used like the object) |
| Can be uninitialized | yes | no, must init at declaration |
| Supports arithmetic | yes | no |

Use a pointer when "no object" (null) or "change what I refer to" is a real
state you need. Use a reference everywhere else — it's harder to misuse.

## Reference categories that matter for interviews

- **Lvalue reference** `T&` — binds to a named, addressable object.
- **Const lvalue reference** `const T&` — binds to lvalues *and* rvalues
  (temporaries); the standard way to pass large objects cheaply without
  allowing mutation.
- **Rvalue reference** `T&&` — binds only to temporaries/moved-from objects;
  this is the mechanism move semantics is built on (see `09-move-semantics`).

## Common traps

- Returning a reference to a local variable — same bug as returning a
  pointer to one, just spelled differently, and arguably easier to miss
  because there's no `&` at the call site reminding you.
- Storing a `const T&` member that's bound to a temporary past that
  temporary's lifetime (member references don't get lifetime extension).
- Assuming `r = y` rebinds `r` — it doesn't; it's an assignment through the
  alias.

## Run it

```bash
./02-references-demo
./02-references-exercises
```
