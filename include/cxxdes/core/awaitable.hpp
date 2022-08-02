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

#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type priority,
    coro_handle current_coro) {
    { t.await_bind(env, priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(current_coro) };
    { t.await_token() } -> std::same_as<token *>;
    { t.await_resume() };
};

template <typename T>
struct immediately_returning_awaitable {
    template <typename ...Args>
    immediately_returning_awaitable(Args &&...args): t_{std::forward<Args>(args)...} {  }

    void await_bind(environment *, priority_type) const noexcept {  }
    bool await_ready() const noexcept { return true; }
    void await_suspend(coro_handle) const noexcept {  }
    token *await_token() const noexcept { return nullptr; }
    T await_resume() { return std::move(t_); }
private:
    T t_;
};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_AWAITABLE_HPP_INCLUDED */
