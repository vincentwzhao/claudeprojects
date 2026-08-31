set(TEST_TARGETS
    test_bump_arena
    test_pool_allocator
    test_free_list_allocator
    test_stl_adapter
    test_stress
)

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    add_test(NAME ${target} COMMAND ${target})
endforeach()
