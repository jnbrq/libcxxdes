/**
 * @file event.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Synchronization primitice event.
 * @date 2022-04-12
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_EVENT_HPP_INCLUDED
#define CXXDES_SYNC_EVENT_HPP_INCLUDED

#include <vector>
#include <stdexcept>

#include <cxxdes/core/process.hpp>

namespace cxxdes {
namespace sync {

namespace detail {
namespace ns_event {

using core::time_type;
using core::priority_type;
using core::coro_handle;
using core::promise_base;
using core::process;

struct event;

struct wake_awaitable {
    event *fence;
    time_type latency;
    priority_type priority;

    core::event *on_suspend(promise_base *promise, coro_handle coro);

    void on_resume() {  }
};

struct wait_awaitable {
    event *fence;
    time_type latency;
    priority_type priority;

    core::event *on_suspend(promise_base *promise, coro_handle coro);

    void on_resume() {  }
};

struct event {
    [[nodiscard("expected usage: co_await fence.wake()")]]
    wake_awaitable wake(time_type latency = 0, priority_type priority = 0) {
        if (waken_) {
            throw std::runtime_error("cannot wake up a waken fence!");
        }

        return {this, latency, priority};
    }

    [[nodiscard("expected usage: co_await fence.wait()")]]
    wait_awaitable wait(time_type latency = 0, priority_type priority = 0) {
        return {this, latency, priority};
    }

    [[nodiscard]]
    bool is_waken() const {
        return waken_;
    }

    void reset() {
        if (waken_) {
            waken_ = false;
        }
        else {
            throw std::runtime_error("cannot reset a not awaken fence!");
        }
    }

    ~event() {
        for (auto e: events_)
            delete e;
        
        events_.clear();
    }
private:
    friend struct wake_awaitable;
    friend struct wait_awaitable;

    bool waken_ = false;

    std::vector<core::event *> events_;
};

inline core::event *wake_awaitable::on_suspend(promise_base *promise, coro_handle coro) {
    for (auto evt: fence->events_) {
        evt->time += promise->env->now();
        promise->env->append_event(evt);
    }

    fence->events_.clear();

    auto evt = new core::event{ promise->env->now() + latency, priority, coro };
    promise->env->append_event(evt);

    fence->waken_ = true;

    return evt;
}

inline core::event *wait_awaitable::on_suspend(promise_base *promise, coro_handle coro) {
    core::event *evt = new core::event{ latency, priority, coro };
    if (fence->waken_) {
        evt->time += promise->env->now();
        promise->env->append_event(evt);
    }
    else {
        fence->events_.push_back(evt);
    }
    return evt;
}

} // namespace ns_event
} // namespace detail

using detail::ns_event::event;

} // namespace sync
} // namespace cxxdes

#endif /* CXXDES_SYNC_EVENT_HPP_INCLUDED */
