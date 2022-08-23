/**
 * @file utils.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Contains utility classes and functions.
 * @date 2022-08-19
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_MISC_UTILS_HPP_INCLUDED
#define CXXDES_MISC_UTILS_HPP_INCLUDED

#define CXXDES_NOT_COPIABLE(TYPE) \
    TYPE(TYPE const &) = delete; \
    TYPE &operator=(TYPE const &) = delete;

#define CXXDES_NOT_MOVABLE(TYPE) \
    TYPE(TYPE &&) = delete; \
    TYPE &operator=(TYPE &&) = delete;

#define CXXDES_DEFAULT_COPIABLE(TYPE) \
    TYPE(TYPE const &) = default; \
    TYPE &operator=(TYPE const &) = default;

#define CXXDES_DEFAULT_MOVABLE(TYPE) \
    TYPE(TYPE &&) = default; \
    TYPE &operator=(TYPE &&) = default;

namespace cxxdes {
namespace util {

struct empty_type {  };

} /* namespace util */
} /* namespace cxxdes */

#endif /* CXXDES_MISC_UTILS_HPP_INCLUDED */
