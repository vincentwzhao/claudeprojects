cmake_minimum_required(VERSION 3.16)
project(cpp_allocators CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# -DSANITIZE=ON -> build with ASan + UBSan. Use this while developing —
# these allocators do raw pointer arithmetic and manual lifetime management,
# exactly the kind of code sanitizers are best at catching bugs in.
option(SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

add_compile_options(-Wall -Wextra -g)

if(SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Benchmarks want real optimization even in an otherwise-Debug sanitizer
# build's CMAKE_BUILD_TYPE, so O2 is applied directly rather than relying on
# CMAKE_BUILD_TYPE=Release (which conflicts with -DSANITIZE=ON in practice).
add_compile_options(-O2)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/common)

enable_testing()

add_subdirectory(tests)
add_subdirectory(benchmarks)
