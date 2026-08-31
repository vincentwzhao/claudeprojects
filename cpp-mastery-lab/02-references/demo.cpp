// 02-references/demo.cpp
#include <cstdio>
#include <string>

void basics() {
    int x = 10;
    int& r = x;        // r is now another name for x
    r = 20;              // this changes x
    printf("x=%d r=%d, &x=%p &r=%p (same address)\n", x, r, (void*)&x, (void*)&r);
}

void rebind_is_not_a_thing() {
    int a = 1, b = 2;
    int& r = a;
    r = b;              // NOT "r now refers to b" — this is a=b, i.e. assigns 2 into a
    printf("a=%d b=%d (a became 2, b unchanged; r still aliases a)\n", a, b);
}

// Pass by const reference: no copy of the string is made, and the callee
// can't modify the caller's object.
void print_len(const std::string& s) {
    printf("len(\"%s\") = %zu\n", s.c_str(), s.size());
}

// const& can bind to a temporary; the temporary's lifetime is extended to
// match the reference.
void lifetime_extension() {
    const std::string& r = std::string("temporary");   // would normally die at ';'
    printf("still alive: %s\n", r.c_str());
}

// Returning a reference to something that outlives the call is fine —
// e.g. returning a reference to a member, or to something passed in by ref.
struct Counter {
    int value = 0;
    int& get() { return value; }   // caller can read AND write through this
};

void reference_to_member() {
    Counter c;
    int& v = c.get();
    v = 42;
    printf("c.value = %d (mutated through returned reference)\n", c.value);
}

int main() {
    printf("-- basics --\n");
    basics();
    printf("\n-- rebind is not a thing --\n");
    rebind_is_not_a_thing();
    printf("\n-- const reference param --\n");
    print_len("hello");
    printf("\n-- lifetime extension --\n");
    lifetime_extension();
    printf("\n-- reference to member --\n");
    reference_to_member();
    return 0;
}
