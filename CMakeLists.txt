cmake_minimum_required(VERSION 3.12)

project(cxxdes)
set(CMAKE_CXX_STANDARD 20)

function (print_var VARNAME)
    message(STATUS "${VARNAME}=${${VARNAME}}")
endfunction ()

# default values to be stored in the cache
set(CXXDES_CLANG_TIDY OFF CACHE BOOL "Use clang tidy.")
set(CXXDES_EXAMPLES ON CACHE BOOL "Build examples.")
set(CXXDES_TESTS ON CACHE BOOL "Build tests.")

set(CXXDES_SANITIZE_ADDRESS ON CACHE BOOL "Enable -sanitize=address for examples and tests.")
set(CXXDES_SANITIZE_LEAK ON CACHE BOOL "Enable -sanitize=leak for examples and tests.")
set(CXXDES_SANITIZE_UNDEFINED ON CACHE BOOL "Enable -sanitize=undefined behavior for examples and tests.")

set(CXXDES_NO_FETCH OFF CACHE BOOL "Do not automatically fetch content.")

message(STATUS "libcxxdes build variables:")
print_var(CXXDES_CLANG_TIDY)
print_var(CXXDES_EXAMPLES)
print_var(CXXDES_TESTS)
print_var(CXXDES_SANITIZE_ADDRESS)
print_var(CXXDES_SANITIZE_LEAK)
print_var(CXXDES_SANITIZE_UNDEFINED)
print_var(CXXDES_NO_FETCH)

if (NOT CXXDES_NO_FETCH)
    # library dependencies
    include(FetchContent)

    # fmt
    FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG master)
    FetchContent_MakeAvailable(fmt)

    # googletest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1)

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
endif ()

# sanitization interface target
add_library(sanitize_config INTERFACE)
if (MSVC)
else ()
    if (CXXDES_SANITIZE_ADDRESS)
        target_compile_options(sanitize_config INTERFACE "-fsanitize=address")
        target_link_options(sanitize_config INTERFACE "-fsanitize=address")
        target_compile_definitions(sanitize_config INTERFACE "-DCXXDES_SANITIZE_ADDRESS")
    endif ()

    if (CXXDES_SANITIZE_LEAK)
        target_compile_options(sanitize_config INTERFACE "-fsanitize=leak")
        target_link_options(sanitize_config INTERFACE "-fsanitize=leak")
        target_compile_definitions(sanitize_config INTERFACE "-DCXXDES_SANITIZE_LEAK")
    endif ()

    if (CXXDES_SANITIZE_UNDEFINED)
        target_compile_options(sanitize_config INTERFACE "-fsanitize=undefined")
        target_link_options(sanitize_config INTERFACE "-fsanitize=undefined")
        target_compile_definitions(sanitize_config INTERFACE "-DCXXDES_SANITIZE_UNDEFINED")
    endif ()
endif ()

# default error levels
if (MSVC)
    # warning level 4
    add_compile_options(/W4) # /WX for warnings as errors
else ()
    # lots of warnings
    add_compile_options(-Wall -Wextra -pedantic) # -Werror for warnings as errors
endif ()

# clang tidy
if (CXXDES_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    set(
        CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXE}"
        "-header-filter=."
        "-checks=-*,modernize-*")
endif ()


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
if(MSVC)
    target_compile_options(cxxdes INTERFACE "/std:c++20")
else ()
    target_compile_options(cxxdes INTERFACE "--std=c++20")
endif ()

# target_compile_definitions(cxxdes INTERFACE "-DMACRO_DEFINITIONS=HERE")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxxdes INTERFACE "-fcoroutines")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxxdes INTERFACE "-fcoroutines-ts")
endif ()

add_library(cxxdes::cxxdes ALIAS cxxdes)
add_library(cxxdes::sanitize_config ALIAS sanitize_config)

if (CXXDES_EXAMPLES)
    file(GLOB example_srcs ${PROJECT_SOURCE_DIR}/examples/*.cpp)

    foreach (src ${example_srcs})
        get_filename_component(basename ${src} NAME_WE)
        add_executable("example_${basename}" "${src}")
        target_link_libraries(
            "example_${basename}"
            PRIVATE
            cxxdes::cxxdes
            fmt::fmt-header-only
            sanitize_config)
    endforeach ()
endif ()

if (CXXDES_TESTS)
    enable_testing()
    file(GLOB test_srcs ${PROJECT_SOURCE_DIR}/tests/*.test.cpp)

    foreach (src ${test_srcs})
        get_filename_component(basename ${src} NAME_WE)
        add_executable("test_${basename}" "${src}")
        target_link_libraries(
            "test_${basename}"
            PRIVATE
            cxxdes::cxxdes
            GTest::gtest_main
            fmt::fmt-header-only
            sanitize_config)
        gtest_discover_tests("test_${basename}")
    endforeach ()
endif ()
