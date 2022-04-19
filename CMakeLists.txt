cmake_minimum_required(VERSION 3.12)

project(cxx-des)
set(CMAKE_CXX_STANDARD 20)

# external libraries
add_subdirectory("external/fmt" EXCLUDE_FROM_ALL)

# Imported library for cxxdes
add_library(cxxdes::cxxdes INTERFACE IMPORTED)
target_include_directories(cxxdes::cxxdes INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_compile_options(cxxdes::cxxdes INTERFACE "--std=c++20")
target_compile_definitions(cxxdes::cxxdes INTERFACE "CXXDES_SAFE")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxxdes::cxxdes INTERFACE "-fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxxdes::cxxdes INTERFACE "-fcoroutines-ts")
endif()

file(GLOB cpp_files ${PROJECT_SOURCE_DIR}/examples/*.cpp)

foreach (cpp_file ${cpp_files})
    get_filename_component(basename ${cpp_file} NAME_WE)
    add_executable("example_${basename}" "${cpp_file}")
    target_link_libraries(
        "example_${basename}"
        PUBLIC
        cxxdes::cxxdes
        fmt::fmt-header-only)
endforeach()

