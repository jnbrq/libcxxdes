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

struct source_location {
    const char * function_name = nullptr;
    const char * file = nullptr;
    unsigned long line = 0;

    [[nodiscard]]
    bool valid() const noexcept {
        return file != nullptr;
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    [[nodiscard]]
    static
    source_location current(
        const char * const function_name = __builtin_FUNCTION(),
        const char * const file = __builtin_FILE(),
        unsigned long const line = __builtin_LINE()
    ) {
        return source_location{ function_name, file, line };
    }
};

namespace detail {

template <typename T>
struct extract_first_type_tag {
    template <typename ...Rest>
    constexpr T operator()(const T &t, const Rest & ...) const {
        return t;
    }

    template <typename Head, typename ...Rest>
    constexpr T operator()(const Head &, const Rest &...rest) const {
        return operator()(rest...);
    }

    constexpr T operator()() const {
        return {};
    }
};

} /* namespace detail */

template <typename T>
inline constexpr detail::extract_first_type_tag<T> extract_first_type;

} /* namespace util */
} /* namespace cxxdes */

#endif /* CXXDES_MISC_UTILS_HPP_INCLUDED */
