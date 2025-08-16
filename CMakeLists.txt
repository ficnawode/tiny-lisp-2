cmake_minimum_required(VERSION 3.25)
project(tiny_lisp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(cmake/packages.cmake)
add_subdirectory(exec)
add_subdirectory(runtime)
add_subdirectory(test)
