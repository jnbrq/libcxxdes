/**
 * @file compositions.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief operator||, operator&& and operator, for awaitables.
 * Also, operator>> for capturing return values.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED
#define CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED

#include <tuple>
#include <iterator>
#include <type_traits>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cxxdes/core/core.hpp>
#include <cxxdes/misc/utils.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_COMPOSITIONS
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

namespace detail {

#include "impl/any_of.ipp"
#include "impl/sequential.ipp"
#include "impl/async.ipp"

} /* namespace detail */

using detail::any_of;
using detail::all_of;
using detail::sequential;
using detail::async;

template <awaitable A1, awaitable A2>
auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
auto operator,(A1 &&a1, A2 &&a2) {
    return sequential(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A, typename Output>
auto operator>>(A &&a, Output &output) -> coroutine<void> {
    output = co_await a;
}

auto flag_done(bool &flag) -> coroutine<void> {
    flag = true;
    co_return ;
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_COMPOSITIONS
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED */
