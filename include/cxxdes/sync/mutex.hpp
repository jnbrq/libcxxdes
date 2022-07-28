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

#include <cxxdes/core/process.hpp>
#include <queue>

namespace cxxdes {
namespace sync {

namespace detail {
namespace ns_mutex {

using core::time_type;
using core::priority_type;
using core::coro_handle;
using core::promise_base;
using core::process;

struct mutex;

struct release_awaitable {
    mutex *mtx;
    time_type latency;
    priority_type priority;

    core::event *on_suspend(promise_base *promise, coro_handle coro);
    void on_resume() {  }
};

struct acquire_awaitable {
    mutex *mtx;
    time_type latency;
    priority_type priority;

    core::event *on_suspend(promise_base *promise, coro_handle coro);
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
        return !coro_;
    }

private:
    friend struct acquire_awaitable;
    friend struct release_awaitable;

    struct event_comp {
        bool operator()(core::event *evt_a, core::event *evt_b) const {
            return (evt_a->priority > evt_b->priority);
        }
    };

    coro_handle coro_ = nullptr;
    std::priority_queue<core::event *, std::vector<core::event *>, event_comp> events_;
};

inline core::event *release_awaitable::on_suspend(promise_base *promise, coro_handle coro) {
    #ifdef CXXDES_SAFE
    if (mtx->coro_ != coro) {
        throw std::runtime_error("cannot release a mutex from a different process.");
    }
    #endif

    // next process to wake up
    if (mtx->events_.size() > 0) {
        auto evt = mtx->events_.top();
        mtx->events_.pop();
        mtx->coro_ = evt->coro;
        evt->time += promise->env->now();
        promise->env->append_event(evt);
    }
    else {
        mtx->coro_ = nullptr;
    }

    // resume the current process
    auto evt = new core::event{ promise->env->now() + latency, priority, coro };
    promise->env->append_event(evt);
    
    return evt;
}

inline core::event *acquire_awaitable::on_suspend(promise_base *promise, coro_handle coro) {
    #ifdef CXXDES_SAFE
    if (mtx->coro_ == coro) {
        throw std::runtime_error("cannot recursively acquire a mutex from the same process.");
    }
    #endif
    
    core::event *evt = new core::event{ latency, priority, coro };
    if (!mtx->coro_) {
        // free mutex
        evt->time += promise->env->now();
        promise->env->append_event(evt);
        mtx->coro_ = coro;
    }
    else {
        // locked mutex, block until we are done
        mtx->events_.push(evt);
    }
    return evt;
}

} /* namespace ns_mutex */
} /* namespace detail */

using detail::ns_mutex::mutex;

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_MUTEX_HPP_INCLUDED */
