/**
 * @file mutex.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Mutex.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_MUTEX_HPP_INCLUDED
#define CXXDES_SYNC_MUTEX_HPP_INCLUDED

#include <queue>
#include <cxxdes/core/process.hpp>
#include <cxxdes/utils.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_SYNC_MUTEX
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace sync {

namespace detail {

using namespace cxxdes::core;

struct mutex;

struct release_awaitable {
    constexpr release_awaitable(
        mutex *mtx,
        time_type latency,
        priority_type priority = priority_consts::inherit):
        mtx_{mtx}, latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(coro_handle current_coro);
    token *await_token() const noexcept { return tkn_; }
    void await_resume() const noexcept {  }

private:
    mutex *mtx_ = nullptr;

    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_type latency_;
    priority_type priority_;
};

struct acquire_awaitable {
    constexpr acquire_awaitable(
        mutex *mtx,
        time_type latency,
        priority_type priority = priority_consts::inherit):
        mtx_{mtx}, latency_{latency}, priority_{priority} {
    }

    CXXDES_NOT_COPIABLE(acquire_awaitable)
    CXXDES_DEFAULT_MOVABLE(acquire_awaitable)

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(coro_handle current_coro);
    token *await_token() const noexcept { return tkn_; }
    void await_resume() const noexcept {  }

private:
    mutex *mtx_ = nullptr;

    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_type latency_;
    priority_type priority_;
};

struct mutex {
    [[nodiscard("expected usage: co_await mtx.acquire()")]]
    auto acquire(time_type latency = 0, priority_type priority = priority_consts::inherit) {
        return acquire_awaitable(this, latency, priority);
    }

    [[nodiscard("expected usage: co_await mtx.release()")]]
    auto release(time_type latency = 0, priority_type priority = priority_consts::inherit) {
        return release_awaitable(this, latency, priority);
    }

    [[nodiscard]]
    bool is_acquired() const {
        return !coro_;
    }

private:
    friend struct acquire_awaitable;
    friend struct release_awaitable;

    struct event_comp {
        bool operator()(token *tkn_a, token *tkn_b) const {
            return (tkn_a->priority > tkn_b->priority);
        }
    };

    coro_handle coro_ = nullptr;
    std::priority_queue<token *, std::vector<token *>, event_comp> tokens_;
};

inline void release_awaitable::await_suspend(coro_handle current_coro) {
    if (mtx_->coro_ != current_coro)
        throw std::runtime_error("cannot release a mutex from a different process.");

    // next process to wake up
    if (mtx_->tokens_.size() > 0) {
        auto evt = mtx_->tokens_.top();
        mtx_->tokens_.pop();
        mtx_->coro_ = evt->coro;
        evt->time += env_->now();
        env_->schedule_token(evt);
    }
    else {
        mtx_->coro_ = nullptr;
    }

    // resume the current process
    tkn_ = new token(env_->now() + latency_, priority_, current_coro);
    env_->schedule_token(tkn_);
}

inline void acquire_awaitable::await_suspend(coro_handle current_coro) {
    if (mtx_->coro_ == current_coro)
        throw std::runtime_error("cannot recursively acquire a mutex from the same process.");
    
    tkn_ = new token(latency_, priority_, current_coro);
    if (!mtx_->coro_) {
        // free mutex
        tkn_->time += env_->now();
        env_->schedule_token(tkn_);
        mtx_->coro_ = current_coro;
    }
    else {
        // locked mutex, block until we are done
        mtx_->tokens_.push(tkn_);
    }
}

} /* namespace detail */

using detail::mutex;

} /* namespace sync */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_SYNC_MUTEX
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_SYNC_MUTEX_HPP_INCLUDED */
