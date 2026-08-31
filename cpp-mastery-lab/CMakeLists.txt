cmake_minimum_required(VERSION 3.16)
project(cpp_mastery_lab CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -DSANITIZE=ON  -> build everything with ASan + UBSan (recommended while
# working through 11-debugging, and generally good hygiene for the rest).
option(SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

add_compile_options(-Wall -Wextra -g)

if(SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/common)

enable_testing()

set(TOPICS
    01-pointers
    02-references
    03-memory-allocation
    04-stack-vs-heap
    05-structs-and-classes
    06-templates
    07-stl
    08-raii
    09-move-semantics
    10-concurrency
    11-debugging
)

foreach(topic ${TOPICS})
    add_subdirectory(${topic})
endforeach()
