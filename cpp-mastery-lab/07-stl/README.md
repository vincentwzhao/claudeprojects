# 07 — STL (containers, iterators, algorithms)

## The concept

The STL is templates (`06-templates`) applied to give you battle-tested,
generic data structures and algorithms so you rarely write raw `new`/`delete`
loops by hand. Interview-relevant complexity table:

| Container | Access | Insert/erase (typical) | Underlying structure |
|---|---|---|---|
| `std::vector<T>` | O(1) random | O(1) amortized at end, O(n) elsewhere | contiguous heap array |
| `std::deque<T>` | O(1) random | O(1) at both ends | chunked array |
| `std::list<T>` | O(n) | O(1) given an iterator | doubly linked list |
| `std::map<K,V>` | O(log n) | O(log n) | balanced BST (red-black) |
| `std::unordered_map<K,V>` | O(1) avg, O(n) worst | O(1) avg | hash table |
| `std::set<T>` | O(log n) | O(log n) | balanced BST |
| `std::unordered_set<T>` | O(1) avg | O(1) avg | hash table |

`std::vector` is the default choice unless you have a specific reason
(need sorted iteration order -> `map`/`set`; need O(1) middle insert/erase
with stable iterators -> `list`; need pure key lookup speed and don't care
about order -> `unordered_map`).

### Iterator invalidation — the sharpest edge

Mutating a container can invalidate iterators/pointers/references into it.
Classic trap:

```cpp
std::vector<int> v = {1,2,3};
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it == 2) v.push_back(99);   // may reallocate -> `it` is now dangling
}
```
`vector::push_back` can trigger reallocation (copy everything to a bigger
buffer), invalidating every iterator/pointer/reference into the old buffer.
`erase` invalidates iterators from the erase point onward. Know each
container's invalidation rules before mutating while iterating — or copy
first, or use the erase-remove idiom / `std::erase_if` (C++20).

### Algorithms + iterators = decoupling

`<algorithm>` functions (`std::sort`, `std::find`, `std::accumulate`, ...)
work on iterator *ranges*, not specific containers — the same `std::sort`
works on a `vector`, a `deque`, or a raw array's begin/end pointers. This
is templates (`06`) doing real work for you.

### Smart pointers live here too

`std::unique_ptr`/`std::shared_ptr` are STL types — covered in depth in
`08-raii`, used freely in this module's examples.

## Common traps

- Using `[]` on `std::map` to *check* existence — it default-inserts a
  missing key. Use `.find()` or `.contains()` (C++20) for lookups you don't
  want to mutate the map.
- Iterator invalidation while mutating during iteration (above).
- Copying large containers by value unintentionally (pass by `const&`).
- Choosing `std::list` "because linked lists are O(1) insert" without
  accounting for cache-unfriendliness — `std::vector` often wins in
  practice even for insert-heavy workloads at moderate sizes.

## Run it

```bash
./07-stl-demo
./07-stl-exercises
```
