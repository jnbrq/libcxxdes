cmake_minimum_required(VERSION 3.12)

project(cxxdes)
set(CMAKE_CXX_STANDARD 20)

# default values to be stored in the cache
set(CXXDES_CLANG_TIDY OFF CACHE BOOL "Use clang tidy.")
set(CXXDES_EXAMPLES ON CACHE BOOL "Build examples.")

# TODO: please see
# https://dominikberner.ch/cmake-interface-lib/

# Library for cxxdes
add_library(cxxdes INTERFACE)
target_include_directories(
    cxxdes
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_options(cxxdes INTERFACE "--std=c++20")
# target_compile_definitions(cxxdes INTERFACE "-DMACRO_DEFINITIONS=HERE")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxxdes INTERFACE "-fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxxdes INTERFACE "-fcoroutines-ts")
endif()

add_library(cxxdes::cxxdes ALIAS cxxdes)

file(GLOB cpp_files ${PROJECT_SOURCE_DIR}/examples/*.cpp)

if (CXXDES_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    set(
        CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXE}"
        "-header-filter=."
        "-checks=-*,modernize-*")
endif()

if (CXXDES_EXAMPLES)

    find_package(fmt)

    foreach (cpp_file ${cpp_files})
        get_filename_component(basename ${cpp_file} NAME_WE)
        add_executable("example_${basename}" "${cpp_file}")
        target_link_libraries(
            "example_${basename}"
            PUBLIC
            cxxdes::cxxdes
            fmt::fmt-header-only)
        
        if (CXXDES_CLANG_TIDY)
            set_target_properties(
                "example_${basename}"
                PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
        endif()
        
        # target_compile_options("example_${basename}" PRIVATE -fsanitize=address)
        # target_link_options("example_${basename}" PRIVATE -fsanitize=address)
    endforeach()

endif()
