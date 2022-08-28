/**
 * @file awaitable.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Defines some convenience awaitables and the awaitable concept.
 * @date 2022-08-03
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_AWAITABLE_HPP_INCLUDED
#define CXXDES_CORE_AWAITABLE_HPP_INCLUDED

#include <type_traits>
#include <concepts>
#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type inherited_priority,
    coro_handle current_coro) {
    { t.await_bind(env, inherited_priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(current_coro) };
    { t.await_token() } -> std::same_as<token *>;
    { t.await_resume() };
};

template <typename T>
concept awaitable_factory = requires(T t) {
    { t.await() } -> awaitable;
};

template <typename T>
struct immediately_returning_awaitable {
    T return_value;

    void await_bind(environment *, priority_type) const noexcept {  }
    bool await_ready() const noexcept { return true; }
    void await_suspend(coro_handle) const noexcept {  }
    token *await_token() const noexcept { return nullptr; }
    T await_resume() { return std::forward<T>(return_value); }
};

// Clang needs a deduction guide
template <typename A>
immediately_returning_awaitable(A &&a) -> immediately_returning_awaitable<std::remove_cvref_t<A>>;

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_AWAITABLE_HPP_INCLUDED */
