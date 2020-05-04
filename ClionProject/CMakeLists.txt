cmake_minimum_required(VERSION 3.13)
project(ex2)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-D_GNU_SOURCE)

add_executable(Smash smash.cpp signals.cpp signals.h Commands.cpp Commands.h main.cpp)



