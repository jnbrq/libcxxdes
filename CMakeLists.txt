cmake_minimum_required(VERSION 3.12)

project(cxx-des)
set(CMAKE_CXX_STANDARD 20)

add_library(cxx_des STATIC "${PROJECT_SOURCE_DIR}/src/event.cpp")
target_include_directories(cxx_des PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_options(cxx_des PUBLIC "--std=c++20")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxx_des PUBLIC "-fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxx_des PUBLIC "-fcoroutines-ts")
endif()

file(GLOB cpp_files ${PROJECT_SOURCE_DIR}/examples/*.cpp)

foreach (cpp_file ${cpp_files})
    get_filename_component(basename ${cpp_file} NAME_WE)
    add_executable("example_${basename}" "${cpp_file}")
    target_link_libraries(
        "example_${basename}"
        PUBLIC
        cxx_des)
endforeach()
