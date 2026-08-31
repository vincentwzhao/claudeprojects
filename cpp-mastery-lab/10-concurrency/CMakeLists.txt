add_executable(10-concurrency-demo demo.cpp)
target_link_libraries(10-concurrency-demo PRIVATE Threads::Threads)

add_executable(10-concurrency-exercises exercises.cpp)
target_link_libraries(10-concurrency-exercises PRIVATE Threads::Threads)
add_test(NAME 10-concurrency-exercises COMMAND 10-concurrency-exercises)
