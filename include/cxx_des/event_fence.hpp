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

#include "process.hpp"

namespace cxx_des {

namespace detail {
namespace event_fence {

struct event_fence;

struct wake_base {
    event_fence *fence;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle);

    void on_resume() {  }
};

struct wait_base {
    event_fence *fence;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle);

    void on_resume() {  }
};

using wake_type = awaitable<wake_base>;
using wait_type = awaitable<wait_base>;

struct event_fence {
    wake_type wake(time_type latency = 0, priority_type priority = 0) {
        return wake_type{this, latency, priority};
    }

    wait_type wait(time_type latency = 0, priority_type priority = 0) {
        return wait_type{this, latency, priority};
    }

    ~event_fence() {
        for (auto e: events_)
            delete e;
        
        events_.clear();
    }
private:
    friend class wake_base;
    friend class wait_base;

    std::vector<event *> events_;
};

inline event *wake_base::on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle) {
    for (auto evt: fence->events_) {
        evt->time += promise.env->now();
        promise.env->append_event(evt);
    }

    fence->events_.clear();

    auto evt = new event{ promise.env->now() + latency, priority, coroutine_handle };
    promise.env->append_event(evt);
    return evt;
}

inline event *wait_base::on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle) {
    auto evt = new event{ latency, priority, coroutine_handle };
    fence->events_.push_back(evt);
    return evt;
}

} // namespace event_fence
} // namespace detail

using detail::event_fence::event_fence;

} // namespace cxx_des

#endif /* CXX_DES_EVENT_FENCE_HPP_INCLUDED */
