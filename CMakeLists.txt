cmake_minimum_required(VERSION 3.12)

project(cxx-des)
set(CMAKE_CXX_STANDARD 20)

add_executable(cxx-des-1 cxx-des-1.cpp)
target_compile_options(cxx-des-1 PRIVATE "--std=c++20" "-fcoroutines")
