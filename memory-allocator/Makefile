CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g -Iinclude -Wno-deprecated-declarations
LDFLAGS := -lpthread

.PHONY: all demo test clean

all: demo test

demo: bin/demo

test: bin/test_allocator
	./bin/test_allocator

bin:
	mkdir -p bin

bin/demo: src/allocator.c src/demo.c include/allocator.h | bin
	$(CC) $(CFLAGS) src/allocator.c src/demo.c -o $@ $(LDFLAGS)

bin/test_allocator: src/allocator.c tests/test_allocator.c include/allocator.h | bin
	$(CC) $(CFLAGS) src/allocator.c tests/test_allocator.c -o $@ $(LDFLAGS)

clean:
	rm -rf bin
