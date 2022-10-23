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

struct exception_container;

struct coroutine_info;
using coroutine_info_ptr = memory::ptr<coroutine_info>;
using const_coroutine_info_ptr = memory::ptr<const coroutine_info>;

struct environment;

template <typename ReturnType, bool Unique>
struct coroutine;

template <typename ReturnType>
struct subroutine;

#include "impl/defs.ipp"
#include "impl/awaitable.ipp"

#include "impl/token.ipp"
#include "impl/immediately_return.ipp"
#include "impl/exception_types.ipp"
#include "impl/exception_container.ipp"
#include "impl/coroutine_info.ipp"
#include "impl/environment.ipp"
#include "impl/await_transform.ipp"
#include "impl/subroutine.ipp"
#include "impl/coroutine.ipp"

} /* namespace core */
} /* namespace cxxdes */

#include "impl/co_with.ipp"
#include "impl/under_coroutine.ipp"

#endif /* CXXDES_CORE_CORE_HPP_INCLUDED */
