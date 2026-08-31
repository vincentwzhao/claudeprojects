cmake_minimum_required(VERSION 3.10)
project(multithreaded_producer_consumer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_executable(producer_consumer src/producer_consumer.cpp)
target_include_directories(producer_consumer PRIVATE include)
target_link_libraries(producer_consumer PRIVATE Threads::Threads)

add_executable(race_condition_demo src/race_condition_demo.cpp)
target_link_libraries(race_condition_demo PRIVATE Threads::Threads)

add_executable(deadlock_demo src/deadlock_demo.cpp)
target_link_libraries(deadlock_demo PRIVATE Threads::Threads)
