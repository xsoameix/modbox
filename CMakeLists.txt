cmake_minimum_required(VERSION 2.8)

set(PROJECT air)

file(GLOB_RECURSE AIR_SOURCES "*.c.c")
add_executable(${PROJECT} ${AIR_SOURCES})
find_package(Threads)
target_link_libraries(${PROJECT} ${CMAKE_THREAD_LIBS_INIT})
