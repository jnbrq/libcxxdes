/**
 * @file helpers.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Macros and utility functions to facilitate debugging the library.
 * @date 2022-08-09
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_DEBUG_HELPERS_HPP_INCLUDED
#define CXXDES_DEBUG_HELPERS_HPP_INCLUDED

#include <utility>
#include <iostream>

struct message_printer {
    std::ostream &out;

    template <typename ...Args>
    auto operator()(Args && ...args) const -> auto const& {
        (out << ... << std::forward<Args>(args));
        return *this;
    }
};

#define CXXDES_MESSAGE(out, label, ...) \
    message_printer{out}("[CXXDES] [", label, "] ", __VA_ARGS__, " (", __FILE__, ":", __LINE__, ")\n");

// to be redefined
#define CXXDES_WARNING(...) ((void) 0)
#define CXXDES_DEBUG(...) ((void) 0)

#define CXXDES_UNSAFE(format, ...) CXXDES_MESSAGE(std::cout, "UNSAFE", format, __VA_ARGS__)
#define CXXDES_ERROR(format, ...) CXXDES_MESSAGE(std::cout, "ERROR", format, __VA_ARGS__)

// TODO use a better solution to emulate __PRETTY_FUNCTION__ on MSVC
#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#   define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define CXXDES_DEBUG_MEMBER_FUNCTION CXXDES_DEBUG("[member function call] this = ", (void *) this, " ", __PRETTY_FUNCTION__)
#define CXXDES_DEBUG_FUNCTION CXXDES_DEBUG("[function call] ", __PRETTY_FUNCTION__)
#define CXXDES_DEBUG_VARIABLE(x) CXXDES_DEBUG("[variable] ", #x, " = ", (x))

#endif /* CXXDES_DEBUG_HELPERS_HPP_INCLUDED */
