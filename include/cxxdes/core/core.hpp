/**
 * @file core.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief coroutine and environment.
 * @date 2022-10-20
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_CORE_HPP_INCLUDED
#define CXXDES_CORE_CORE_HPP_INCLUDED

#include <type_traits>
#include <concepts>
#include <stdexcept>
#include <optional>
#include <queue>
#include <unordered_set>
#include <cstddef>
#include <memory>
#include <stack>
#include <string>

#include <cxxdes/misc/time.hpp>
#include <cxxdes/misc/utils.hpp>
#include <cxxdes/misc/reference_counted.hpp>
#include <cxxdes/misc/time.hpp>

#if __has_include(<coroutine>)

#include <coroutine>

#elif __has_include(<experimental/coroutine>)

#include <experimental/coroutine>

namespace std {
    using namespace experimental;
}

#else

#error "neither <coroutine> nor <experimental/coroutine> found!"

#endif

namespace cxxdes {
namespace core {

namespace detail {
    template <typename Derived>
    struct await_ops_mixin;
}

struct token_handler;
struct token;

template <typename T>
struct immediately_return;

struct interrupted_exception;
struct stopped_exception;

struct coroutine_data;
using coroutine_data_ptr = memory::ptr<coroutine_data>;
using const_coroutine_data_ptr = memory::ptr<const coroutine_data>;

struct environment;

template <typename ReturnType = void, bool Unique = false>
struct coroutine;

template <typename ReturnValue = void>
using unique_coroutine = coroutine<ReturnValue, true>;

template <typename ReturnType>
struct subroutine;

#include "impl/defs.ipp"
#include "impl/awaitable.ipp"

#include "impl/token.ipp"
#include "impl/immediately_return.ipp"
#include "impl/exception_types.ipp"
#include "impl/coroutine_data.ipp"
#include "impl/environment.ipp"
#include "impl/timeout.ipp"
#include "impl/await_transform.ipp"
#include "impl/subroutine.ipp"
#include "impl/coroutine.ipp"
#include "impl/compositions.ipp"

} /* namespace core */
} /* namespace cxxdes */

#include "impl/co_with.ipp"
#include "impl/under_coroutine.ipp"

#endif /* CXXDES_CORE_CORE_HPP_INCLUDED */
