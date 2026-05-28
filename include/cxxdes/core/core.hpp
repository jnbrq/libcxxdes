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

#include <fmt/core.h>

#include <type_traits>
#include <concepts>
#include <stdexcept>
#include <optional>
#include <queue>
#include <unordered_set>
#include <cstddef>
#include <memory>
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

/** @brief Base interface for token handlers used by control-flow composition. */
struct token_handler;

/** @brief Scheduled resume point owned through intrusive reference counting. */
struct token;

template <typename T>
struct immediately_return;

/** @brief Shared state behind one schedulable coroutine process. */
struct coroutine_data;

/** @brief Intrusive pointer to mutable coroutine state. */
using coroutine_data_ptr = memory::ptr<coroutine_data>;

/** @brief Intrusive pointer to const coroutine state. */
using const_coroutine_data_ptr = memory::ptr<const coroutine_data>;

/** @brief Simulation environment that owns time and the event queue. */
struct environment;

/**
 * @brief Schedulable coroutine process handle.
 *
 * @tparam ReturnType Value produced by the coroutine, or `void`.
 * @tparam Unique Whether awaiting moves the stored return value.
 */
template <typename ReturnType = void, bool Unique = false>
struct coroutine;

/** @brief Move-only coroutine process handle with single-consumer return values. */
template <typename ReturnValue = void>
using unique_coroutine = coroutine<ReturnValue, true>;

/** @brief Coroutine helper frame that runs inside the current process. */
template <typename ReturnType>
struct subroutine;

#include "impl/defs.ipp"
#include "impl/awaitable.ipp"

#include "impl/token.ipp"
#include "impl/immediately_return.ipp"
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
