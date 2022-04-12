/**
 * @file coroutine.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Includes the coroutine headers for Clang and gcc.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_COROUTINE_HPP_INCLUDED
#define CXX_DES_COROUTINE_HPP_INCLUDED

#if (defined(__APPLE__) && defined(__clang__) && ___clang_major__ <= 13) || (defined(__clang__) && __clang_major__ <= 13)
#include <experimental/coroutine>

namespace std {
using namespace experimental;
}

#else
#include <coroutine>
#endif

#include <exception>

#endif // CXX_DES_COROUTINE_HPP_INCLUDED