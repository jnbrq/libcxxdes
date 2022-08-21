cmake_minimum_required(VERSION 3.12)

project(cxxdes)
set(CMAKE_CXX_STANDARD 20)

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

# default values to be stored in the cache
set(CXXDES_CLANG_TIDY OFF CACHE BOOL "Use clang tidy.")
set(CXXDES_EXAMPLES ON CACHE BOOL "Build examples.")
set(CXXDES_TESTS ON CACHE BOOL "Build tests.")

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
else()
    target_compile_options(cxxdes INTERFACE "--std=c++20")
endif()

# target_compile_definitions(cxxdes INTERFACE "-DMACRO_DEFINITIONS=HERE")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cxxdes INTERFACE "-fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cxxdes INTERFACE "-fcoroutines-ts")
endif()

add_library(cxxdes::cxxdes ALIAS cxxdes)


if (CXXDES_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    set(
        CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXE}"
        "-header-filter=."
        "-checks=-*,modernize-*")
endif()

if (CXXDES_EXAMPLES)
    file(GLOB example_srcs ${PROJECT_SOURCE_DIR}/examples/*.cpp)

    foreach (src ${example_srcs})
        get_filename_component(basename ${src} NAME_WE)
        add_executable("example_${basename}" "${src}")
        target_link_libraries(
            "example_${basename}"
            PRIVATE
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
            fmt::fmt-header-only)
        gtest_discover_tests("test_${basename}")

        if (MSVC)
            # TODO figure out a way to enable address sanitizer for MSVC+cmake
        else()
            target_compile_options(
                "test_${basename}"
                PRIVATE
                "-fsanitize=address"
                "-fsanitize=undefined")
            
            target_link_options(
                "test_${basename}"
                PRIVATE
                "-fsanitize=address"
                "-fsanitize=undefined")
            # TODO verify the link flag: -fsanitize=undefined

            if (NOT APPLE)
                target_compile_options(
                    "test_${basename}"
                    PRIVATE
                    "-fsanitize=leak")
                
                target_link_options(
                    "test_${basename}"
                    PRIVATE
                    "-fsanitize=leak")
            endif()
        endif()

        if (CXXDES_CLANG_TIDY)
            set_target_properties(
                "test_${basename}"
                PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
        endif()
    endforeach()
endif()
