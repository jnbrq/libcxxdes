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
#include <cxxdes/core/core.hpp>

namespace cxxdes {
namespace sync {

namespace detail {

using namespace cxxdes::core;

struct event;

struct wake_awaitable {
    constexpr wake_awaitable(event *evt):
        evt_{evt} {
    }

    void await_bind(environment *env, priority_type) noexcept {
        env_ = env;
    }

    bool await_ready();
    void await_suspend(coroutine_data_ptr) const noexcept {  }
    token *await_token() const noexcept { return nullptr; }
    void await_resume(no_return_value_tag = {}) const noexcept {  }

private:
    event *evt_ = nullptr;
    environment *env_ = nullptr;
};

struct wait_awaitable {
    constexpr wait_awaitable(
        event *evt,
        time_integral latency,
        priority_type priority = priority_consts::inherit):
        evt_{evt}, latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(coroutine_data_ptr coro_data);
    token *await_token() const noexcept { return tkn_; }
    void await_resume(no_return_value_tag = {}) const noexcept {  }

private:
    event *evt_ = nullptr;

    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_integral latency_;
    priority_type priority_;
};

/**
 * @brief One-shot wake-up point for coroutines currently waiting on it.
 *
 * `wait()` suspends the current process until a later `wake()` schedules all
 * waiters currently registered with the event. A wake does not persist; a
 * process that starts waiting after a wake must wait for a later wake.
 *
 * The event owns waiter tokens while processes are blocked on it, so the event
 * must outlive all blocked waiters unless the whole environment is being torn
 * down.
 */
struct event {
    /**
     * @brief Returns an awaitable that wakes all current waiters.
     *
     * The waiters are scheduled relative to the current environment time using
     * the latency and priority captured by each `wait()` call.
     */
    [[nodiscard("expected usage: co_await event.wake()")]]
    auto wake() {
        return wake_awaitable(this);
    }

    /**
     * @brief Returns an awaitable that waits for the next wake.
     *
     * @param latency Additional delay, in simulation ticks, applied after the
     *        wake time before this waiter resumes.
     * @param priority Resume priority for this waiter, or
     *        `priority_consts::inherit`.
     */
    [[nodiscard("expected usage: co_await event.wait()")]]
    auto wait(time_integral latency = 0, priority_type priority = priority_consts::inherit) {
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

inline bool wake_awaitable::await_ready() {
    for (auto tkn: evt_->tokens_) {
        tkn->time += env_->now();
        env_->schedule_token(tkn);
    }

    evt_->tokens_.clear();

    return true;
}

inline void wait_awaitable::await_suspend(coroutine_data_ptr coro_data) {
    tkn_ = new token(latency_, priority_, coro_data, "woke up");
    evt_->tokens_.push_back(tkn_);
}

} /* namespace detail */

using detail::event;

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_EVENT_HPP_INCLUDED */
