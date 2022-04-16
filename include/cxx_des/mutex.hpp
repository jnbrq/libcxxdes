/**
 * @file mutex.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Mutex.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_MUTEX_HPP_INCLUDED
#define CXX_DES_MUTEX_HPP_INCLUDED

#include "process.hpp"
#include <queue>

namespace cxx_des {

namespace detail {
namespace mutex {

struct mutex;

struct release_awaitable {
    mutex *mtx;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle);
    void on_resume() {  }
};

struct acquire_awaitable {
    mutex *mtx;
    time_type latency;
    priority_type priority;

    event *on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle);
    void on_resume() {  }
};

struct mutex {
    [[nodiscard("expected usage: co_await mtx.acquire()")]]
    acquire_awaitable acquire(time_type latency = 0, priority_type priority = 0) {
        return {this, latency, priority};
    }

    [[nodiscard("expected usage: co_await mtx.release()")]]
    release_awaitable release(time_type latency = 0, priority_type priority = 0) {
        return {this, latency, priority};
    }

    [[nodiscard]]
    bool is_acquired() const {
        return !coroutine_handle_;
    }

private:
    friend struct acquire_awaitable;
    friend struct release_awaitable;

    struct event_comp {
        bool operator()(event *evt_a, event *evt_b) const {
            return (evt_a->priority > evt_b->priority);
        }
    };

    std::coroutine_handle<> coroutine_handle_ = nullptr;
    std::priority_queue<event *, std::vector<event *>, event_comp> events_;
};

inline event *release_awaitable::on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle) {
    #if CXX_DES_SAFE
    if (mtx->coroutine_handle_ != coroutine_handle) {
        throw std::runtime_error("cannot release a mutex from a different process.");
    }
    #endif

    // next process to wake up
    if (mtx->events_.size() > 0) {
        auto evt = mtx->events_.top();
        mtx->events_.pop();
        mtx->coroutine_handle_ = evt->coroutine_handle;
        evt->time += promise->env->now();
        promise->env->append_event(evt);
    }
    else {
        mtx->coroutine_handle_ = nullptr;
    }

    // resume the current process
    auto evt = new event{ promise->env->now() + latency, priority, coroutine_handle };
    promise->env->append_event(evt);
    
    return evt;
}

inline event *acquire_awaitable::on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle) {
    #ifdef CXX_DES_SAFE
    if (mtx->coroutine_handle_ == coroutine_handle) {
        throw std::runtime_error("cannot recursively acquire a mutex from the same process.");
    }
    #endif
    
    event *evt = new event{ latency, priority, coroutine_handle };
    if (!mtx->coroutine_handle_) {
        // free mutex
        evt->time += promise->env->now();
        promise->env->append_event(evt);
        mtx->coroutine_handle_ = coroutine_handle;
    }
    else {
        // locked mutex, block until we are done
        mtx->events_.push(evt);
    }
    return evt;
}

} // namespace mutex
} // namespace detail

using detail::mutex::mutex;

} // namespace cxx_des

#endif /* CXX_DES_MUTEX_HPP_INCLUDED */
