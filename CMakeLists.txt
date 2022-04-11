cmake_minimum_required(VERSION 3.12)

project(cxx-des)
set(CMAKE_CXX_STANDARD 20)

add_executable(cxx-des-1 cxx-des-1.cpp)
target_include_directories(cxx-des-1 PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(cxx-des-1 PRIVATE "--std=c++20")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxx-des-1 PRIVATE "-fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxx-des-1 PRIVATE "-fcoroutines-ts")
endif()
