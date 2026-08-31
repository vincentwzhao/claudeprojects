// 08-raii/demo.cpp
#include <cstdio>
#include <memory>
#include <stdexcept>

class LoudResource {
    int id_;
public:
    explicit LoudResource(int id) : id_(id) {
        printf("  [acquire %d]\n", id_);
    }
    ~LoudResource() {
        printf("  [release %d]\n", id_);
    }
};

void exception_safety_demo() {
    printf("  entering scope\n");
    LoudResource r(1);
    try {
        LoudResource r2(2);
        throw std::runtime_error("something went wrong");
        // r2's destructor runs during stack unwinding, before the catch
        // handler below is even entered — RAII, not a try/finally.
    } catch (const std::exception& e) {
        printf("  caught: %s (note: r2 was already released above)\n", e.what());
    }
    printf("  leaving scope (r released after this)\n");
}   // r released here

void unique_ptr_demo() {
    auto p = std::make_unique<LoudResource>(10);
    printf("  using p...\n");
}   // p's destructor runs -> LoudResource destructor runs -> "release 10"

void shared_ptr_demo() {
    std::shared_ptr<LoudResource> a = std::make_shared<LoudResource>(20);
    printf("  refcount after creation: %ld\n", a.use_count());
    {
        std::shared_ptr<LoudResource> b = a;   // shares ownership
        printf("  refcount with b alive: %ld\n", a.use_count());
    }   // b destroyed, but resource is NOT released (a still owns it)
    printf("  refcount after b's scope ends: %ld\n", a.use_count());
}   // a destroyed here -> refcount hits 0 -> resource released

struct Node {
    int value;
    std::shared_ptr<Node> next;       // owning: keeps the chain alive
    std::weak_ptr<Node> prev;          // non-owning: avoids a reference cycle
    explicit Node(int v) : value(v) {}
    ~Node() { printf("  ~Node(%d)\n", value); }
};

void weak_ptr_breaks_cycles() {
    auto a = std::make_shared<Node>(1);
    auto b = std::make_shared<Node>(2);
    a->next = b;
    b->prev = a;   // weak_ptr: does NOT bump a's refcount
    printf("  a.use_count()=%ld b.use_count()=%ld\n", a.use_count(), b.use_count());
    // If b->prev were a shared_ptr<Node> instead, a and b would keep each
    // other alive forever (refcount never reaches 0) — a leak. With
    // weak_ptr, both are destroyed normally when a and b go out of scope.
}

int main() {
    printf("-- exception safety: destructors run during unwinding --\n");
    exception_safety_demo();

    printf("\n-- unique_ptr: sole owner, destroyed with scope --\n");
    unique_ptr_demo();

    printf("\n-- shared_ptr: refcounted shared ownership --\n");
    shared_ptr_demo();

    printf("\n-- weak_ptr: breaking a reference cycle --\n");
    weak_ptr_breaks_cycles();

    return 0;
}
