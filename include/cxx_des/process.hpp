/**
 * @file process.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Process class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_PROCESS_HPP_INCLUDED
#define CXX_DES_PROCESS_HPP_INCLUDED

#include <concepts>
#include <stdexcept>

#include "event.hpp"
#include "environment.hpp"

namespace cxx_des {

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
         * @brief Unhandled exception.
         * 
         */
        std::exception_ptr exception = nullptr;

        /**
         * @brief Event scheduled when this process returns.
         * 
         */
        event *completion_evt = nullptr;

        bool started = false;

        // function coroutines
        template <typename ...Args>
        promise_type(environment *env, Args && ...args): env{env} {  }

        // class coroutines
        template <typename ...Args>
        promise_type(process_class auto& t, Args && ...args): promise_type{&t.env} {  }

        event *start() {
            if (!started) {
                auto handle = handle_type::from_promise(*this);
                env->register_coroutine(handle);
                auto evt = new event{ 0, -1000, handle };
                env->append_event(evt);
                started = true;
                return evt;
            }
            return nullptr;
        }
        
        process get_return_object() {
            return process(handle_type::from_promise(*this));
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception = std::current_exception(); }

        void return_void() {
            if (completion_evt) {
                completion_evt->time += env->now();
                env->append_event(completion_evt);
            }
        }

        template <typename T>
        auto await_transform(T &&a);
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

        if (auto e = handle_.promise().exception) {
            std::rethrow_exception(e);
        }

        return false;
    }

    static promise_type *promise_of(std::coroutine_handle<> coroutine_handle) {
        return &(process::handle_type::from_address(coroutine_handle.address()).promise());
    }

    // process is also awaitable
    event *on_suspend(promise_type *promise, std::coroutine_handle<> other_handle) {
        auto this_promise = promise_of(handle_);

        // start if deferred
        this_promise->start();

        // in case of completion, trigger the currently paused coroutine
        event *completion_evt = new event{ 0, 1000, other_handle };
        this_promise->completion_evt = completion_evt;
        return completion_evt;
    }

    void on_resume() {  }

    auto &start() {
        promise_of(handle_)->start();
        return *this;
    }

private:
    handle_type handle_;
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
        #ifdef CXX_DES_DEBUG
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

} // namespace cxx_des

#endif /* CXX_DES_PROCESS_HPP_INCLUDED */
