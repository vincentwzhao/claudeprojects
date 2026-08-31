// 09-move-semantics/demo.cpp
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

class Buffer {
    char* data_ = nullptr;
    size_t size_ = 0;
    const char* tag_;   // just for logging which instance is which

public:
    Buffer(const char* tag, const char* s) : tag_(tag) {
        size_ = strlen(s);
        data_ = new char[size_ + 1];
        memcpy(data_, s, size_ + 1);
        printf("  [%s] constructed (\"%s\")\n", tag_, data_);
    }

    // copy: expensive — allocates + copies bytes
    Buffer(const Buffer& other) : tag_("copy") {
        size_ = other.size_;
        data_ = new char[size_ + 1];
        memcpy(data_, other.data_, size_ + 1);
        printf("  [copy ctor] deep-copied \"%s\" (expensive)\n", data_);
    }

    // move: cheap — steal the pointer, null out the source
    Buffer(Buffer&& other) noexcept : tag_("moved") {
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        printf("  [move ctor] stole \"%s\"'s buffer (cheap, no allocation)\n", data_ ? data_ : "(null)");
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~Buffer() {
        printf("  [%s] destroyed (data=%s)\n", tag_, data_ ? data_ : "(null, already moved-from)");
        delete[] data_;
    }

    const char* c_str() const { return data_ ? data_ : "(null)"; }
};

void copy_vs_move() {
    Buffer a("a", "hello world");
    printf("\n  -- copying a into b --\n");
    Buffer b = a;                       // copy constructor: deep copy
    printf("\n  -- moving a into c --\n");
    Buffer c = std::move(a);            // move constructor: steal, a is now empty
    printf("\n  a is now: \"%s\" (moved-from — don't rely on its value)\n", a.c_str());
    printf("  b is still: \"%s\" (independent copy)\n", b.c_str());
    printf("  c is now:   \"%s\" (stole a's buffer)\n", c.c_str());
}

void vector_reallocation_uses_move() {
    printf("\n  pushing into a vector that will need to reallocate:\n");
    std::vector<Buffer> v;
    v.reserve(1);                       // force a reallocation on the 2nd push
    v.emplace_back("v0", "first");
    printf("  -- pushing a second element triggers reallocation --\n");
    v.emplace_back("v1", "second");
    printf("  (existing elements were MOVED into the new buffer, not copied,\n");
    printf("   because the move constructor is marked noexcept)\n");
}

int main() {
    printf("-- copy vs move --\n");
    copy_vs_move();
    printf("\n-- vector reallocation prefers move over copy --\n");
    vector_reallocation_uses_move();
    return 0;
}
