/**
 * @file debug_helpers.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Macros and utility functions to facilitate debugging the library.
 * @date 2022-08-08
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_DEBUG_HELPERS_HPP_INCLUDED
#define CXXDES_CORE_DEBUG_HELPERS_HPP_INCLUDED

#include <stdio.h>
#include <fmt/core.h>
#include <string_view>
#include <string>

#define CXXDES_MESSAGE(file, label, format, ...) \
    fmt::print((file), "[CXXDES] [{}] " format "\n", (label), __VA_ARGS__)

#ifdef CXXDES_LIBRARY_SAFE
#   define CXXDES_WARNING(format, ...) CXXDES_MESSAGE(stdout, "WARNING", format, __VA_ARGS__)
#else
#   define CXXDES_WARNING(...) ((void) 0)
#endif

#ifdef CXXDES_LIBRARY_DEBUG
#   define CXXDES_DEBUG(format, ...) CXXDES_MESSAGE(stdout, "DEBUG", format, __VA_ARGS__)
#else
#   define CXXDES_DEBUG(...) ((void) 0)
#endif

#define CXXDES_ERROR(format, ...) CXXDES_MESSAGE(stdout, "ERROR", format, __VA_ARGS__)

// TODO use a better solution to emulate __PRETTY_FUNCTION__ on MSVC
#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#   define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define CXXDES_DEBUG_MEMBER_FUNCTION CXXDES_DEBUG("this = {} {}", (void *) this, __PRETTY_FUNCTION__)
#define CXXDES_DEBUG_FUNCTION CXXDES_DEBUG("{}", __PRETTY_FUNCTION__)

#endif /* CXXDES_CORE_DEBUG_HELPERS_HPP_INCLUDED */
