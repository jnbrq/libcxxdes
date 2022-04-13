/**
 * @file event_fence.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief 
 * @date 2022-04-12
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_EVENT_FENCE_HPP_INCLUDED
#define CXX_DES_EVENT_FENCE_HPP_INCLUDED

#include <vector>
#include <stdexcept>

#include "process.hpp"

namespace cxx_des {

namespace detail {
namespace ns_event_fence {

struct event_fence;

struct wake_type {
    event_fence *fence;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle);

    void on_resume() {  }
};

struct wait_type {
    event_fence *fence;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle);

    void on_resume() {  }
};

struct event_fence {
    [[nodiscard("expected usage: co_await fence.wake()")]]
    wake_type wake(time_type latency = 0, priority_type priority = 0) {
        if (waken_) {
            throw std::runtime_error("cannot wake up a waken fence!");
        }

        return wake_type{this, latency, priority};
    }

    [[nodiscard("expected usage: co_await fence.wait()")]]
    wait_type wait(time_type latency = 0, priority_type priority = 0) {
        return wait_type{this, latency, priority};
    }

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

    ~event_fence() {
        for (auto e: events_)
            delete e;
        
        events_.clear();
    }
private:
    friend class wake_type;
    friend class wait_type;

    bool waken_ = false;

    std::vector<event *> events_;
};

inline event *wake_type::on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle) {
    for (auto evt: fence->events_) {
        evt->time += promise.env->now();
        promise.env->append_event(evt);
    }

    fence->events_.clear();

    auto evt = new event{ promise.env->now() + latency, priority, coroutine_handle };
    promise.env->append_event(evt);

    fence->waken_ = true;

    return evt;
}

inline event *wait_type::on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle) {
    event *evt = new event{ latency, priority, coroutine_handle };
    if (fence->waken_) {
        evt->time += promise.env->now();
        promise.env->append_event(evt);
    }
    else {
        fence->events_.push_back(evt);
    }
    return evt;
}

} // namespace ns_event_fence
} // namespace detail

using detail::ns_event_fence::event_fence;

} // namespace cxx_des

#endif /* CXX_DES_EVENT_FENCE_HPP_INCLUDED */
