/**
 * @file process.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Process class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_PROCESS_HPP_INCLUDED
#define CXXDES_CORE_PROCESS_HPP_INCLUDED

#include <concepts>
#include <stdexcept>

#include <cxxdes/core/event.hpp>
#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

namespace detail {
namespace ns_process {

template <typename T>
concept process_class = requires(T a) {
    { &a.env } -> std::same_as<environment *>;
};

struct process {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        /**
         * @brief the environment that is associated with the event object.
         * 
         */
        environment *env;

        /**
         * @brief Event scheduled when this process returns.
         * 
         */
        event *completion_evt = nullptr;

        /**
         * @brief Event schedule for starting the process. nullptr if scheduled.
         * 
         */
        event *start_event = nullptr;

        template <typename ...Args>
        promise_type(Args && ...args): env{env} {
            handle_ = handle_type::from_promise(*this);
            start_event = new event{0, -1000, handle_};
        }

        void start(environment *env) {
            if (start_event) {
                this->env = env;
                env->register_coroutine(handle_);
                env->append_event(start_event);
                start_event = nullptr;
            }
        }
        
        process get_return_object() {
            return process(handle_);
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {
            std::rethrow_exception(std::current_exception());
        }

        void return_void() {
            if (completion_evt) {
                completion_evt->time += env->now();
                env->append_event(completion_evt);
                completion_evt = nullptr;
            }
        }

        template <typename T>
        auto await_transform(T &&a);

        ~promise_type() {
            if (start_event) delete start_event;
        }

    private:
        handle_type handle_;
    };

    process(handle_type handle): handle_{handle} {  }

    bool done() {
        return handle_.done();
    }

    std::coroutine_handle<> handle() {
        return handle_;
    }

    [[deprecated]]
    bool operator()() {
        if (done())
            return true;
        
        handle_();

        return false;
    }

    static promise_type *promise_of(std::coroutine_handle<> coroutine_handle) {
        return &(process::handle_type::from_address(coroutine_handle.address()).promise());
    }

    promise_type *this_promise() const {
        if (!promise_)
            promise_ = promise_of(handle_);
        
        return promise_;
    }

    // process is also awaitable
    event *on_suspend(promise_type *promise, std::coroutine_handle<> other_handle) {
        // start if deferred
        this_promise()->start(promise->env);

        // in case of completion, trigger the currently paused coroutine
        event *completion_evt = new event{ 0, 1000, other_handle };
        this_promise()->completion_evt = completion_evt;
        return completion_evt;
    }

    void on_resume() {  }

    auto &start(environment &env) {
        this_promise()->start(&env);
        return *this;
    }

    auto &priority(priority_type priority) {
        #ifdef CXX_DES_SAFE
        if (!this_promise()->start_event)
            throw std::runtime_error("cannot change the priority of a started process");
        #endif
        this_promise()->start_event->priority = priority;
        return *this;
    }

    auto &latency(time_type latency) {
        #ifdef CXX_DES_SAFE
        if (!this_promise()->start_event)
            throw std::runtime_error("cannot change the latency of a started process");
        #endif
        this_promise()->start_event->time = latency;
        return *this;
    }

private:
    handle_type handle_ = nullptr;
    mutable promise_type *promise_ = nullptr;
};

template <typename T>
concept awaitable = requires(T t, process::promise_type *promise, std::coroutine_handle<> handle) {
    { t.on_suspend(promise, handle) } -> std::convertible_to<event *>;
    { t.on_resume() };
};

template <awaitable T>
struct wrap_awaitable: T {
    
    template <typename ...U>
    wrap_awaitable(U && ...u): T{std::forward<U>(u)...} {  }

    bool await_ready() {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) {
        auto promise = process::promise_of(handle);
        auto e = T::on_suspend(promise, handle);
        #ifdef CXX_DES_SAFE
        if (e->handler) {
            throw std::runtime_error("a resuming event is required!");
        }
        #endif
        return true;
    }

    auto await_resume() {
        return T::on_resume();
    }

};

template <typename T>
inline auto process::promise_type::await_transform(T &&t) {
    return wrap_awaitable<std::unwrap_ref_decay_t<T>>(std::forward<T>(t));
}

} // namespace ns_process
} // namespace detail

using detail::ns_process::process;
using detail::ns_process::awaitable;
using detail::ns_process::process_class;

} // namespace core
} // namespace cxxdes

#endif /* CXXDES_CORE_PROCESS_HPP_INCLUDED */
