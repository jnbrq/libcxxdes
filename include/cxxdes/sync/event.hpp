/**
 * @file event.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Synchronization primitive event.
 * @date 2022-04-12
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_EVENT_HPP_INCLUDED
#define CXXDES_SYNC_EVENT_HPP_INCLUDED

#include <vector>
#include <stdexcept>
#include <cxxdes/core/token.hpp>
#include <cxxdes/utils.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_SYNC_EVENT
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace sync {

namespace detail {

using namespace cxxdes::core;

struct event;

struct wake_awaitable {
    constexpr wake_awaitable(
        event *evt,
        time_type latency,
        priority_type priority = priority_consts::inherit):
        evt_{evt}, latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(coro_handle current_coro);
    token *await_token() const noexcept { return tkn_; }

    void await_resume() const noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;
    }

private:
    event *evt_ = nullptr;

    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_type latency_;
    priority_type priority_;
};

struct wait_awaitable {
    constexpr wait_awaitable(
        event *evt,
        time_type latency,
        priority_type priority = priority_consts::inherit):
        evt_{evt}, latency_{latency}, priority_{priority} {
    }

    CXXDES_NOT_COPIABLE(wait_awaitable)
    CXXDES_DEFAULT_MOVABLE(wait_awaitable)

    void await_bind(environment *env, priority_type priority) noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(coro_handle current_coro);
    token *await_token() const noexcept { return tkn_; }

    void await_resume() const noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;
    }

private:
    event *evt_ = nullptr;

    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_type latency_;
    priority_type priority_;
};

struct event {
    [[nodiscard("expected usage: co_await event.wake()")]]
    auto wake(time_type latency = 0, priority_type priority = priority_consts::inherit) {
        return wake_awaitable(this, latency, priority);
    }

    [[nodiscard("expected usage: co_await event.wait()")]]
    auto wait(time_type latency = 0, priority_type priority = priority_consts::inherit) {
        return wait_awaitable(this, latency, priority);
    }

    ~event() {
        for (auto tkn: tokens_)
            delete tkn;
        
        tokens_.clear();
    }
private:
    friend struct wake_awaitable;
    friend struct wait_awaitable;

    std::vector<token *> tokens_;
};

inline void wake_awaitable::await_suspend(coro_handle current_coro) {
    CXXDES_DEBUG_MEMBER_FUNCTION;

    for (auto tkn: evt_->tokens_) {
        tkn->time += env_->now();
        env_->schedule_token(tkn);
    }

    evt_->tokens_.clear();

    tkn_ = new token(env_->now() + latency_, priority_, current_coro);
    env_->schedule_token(tkn_);
}

inline void wait_awaitable::await_suspend(coro_handle current_coro) {
    CXXDES_DEBUG_MEMBER_FUNCTION;
    
    tkn_ = new token(latency_, priority_, current_coro);
    evt_->tokens_.push_back(tkn_);
}

} /* namespace detail */

using detail::event;

} /* namespace sync */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_SYNC_EVENT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_SYNC_EVENT_HPP_INCLUDED */
