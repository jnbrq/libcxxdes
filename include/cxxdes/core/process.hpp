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

#include <type_traits>
#include <concepts>
#include <stdexcept>
#include <optional>

#include <cxxdes/core/event.hpp>
#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

namespace detail {
namespace ns_process {

struct get_env_type {};

struct promise_base {
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

    promise_base() {
        start_event = new event{0, -1000, nullptr};
    }

    void start(environment *env) {
        if (start_event) {
            this->env = env;
            start_event->coro = coro_;
            env->append_event(start_event);
            start_event = nullptr;
        }
    }

    std::suspend_always initial_suspend() {
        return {};
    }

    std::suspend_never final_suspend() noexcept {
        return {};
    }

    void unhandled_exception() {
        std::rethrow_exception(std::current_exception());
    }

    template <typename T>
    auto await_transform(T &&a);

    auto await_transform(get_env_type);

    ~promise_base() {
        if (start_event) delete start_event;
    }

protected:
    void do_return() {
        if (completion_evt) {
            completion_evt->time += env->now();
            env->append_event(completion_evt);
            completion_evt = nullptr;
        }
    }

    coro_handle coro_;
};

struct process_base {
    process_base() = default;

    process_base(coro_handle coro, promise_base *promise):
        coro_{coro},
        promise_{promise} {
    }

    bool done() {
        return coro_.done();
    }

    coro_handle get_coro_handle() {
        return coro_;
    }

    promise_base *this_promise() const {
        return promise_;
    }

    // process is also awaitable
    event *on_suspend(promise_base *promise, coro_handle other_coro) {
        // start if deferred
        this_promise()->start(promise->env);

        // in case of completion, trigger the currently paused coroutine
        event *completion_evt = new event{ 0, 1000, other_coro };
        this_promise()->completion_evt = completion_evt;
        return completion_evt;
    }

    void start(environment &env) {
        this_promise()->start(&env);
    }

    void start(environment *env) {
        this_promise()->start(env);
    }

    void priority(priority_type priority) {
        #ifdef CXXDES_SAFE
        if (!this_promise()->start_event)
            throw std::runtime_error("cannot change the priority of a started process");
        #endif
        this_promise()->start_event->priority = priority;
    }

    void latency(time_type latency) {
        #ifdef CXXDES_SAFE
        if (!this_promise()->start_event)
            throw std::runtime_error("cannot change the latency of a started process");
        #endif
        this_promise()->start_event->time = latency;
    }

protected:
    coro_handle coro_ = nullptr;
    mutable promise_base *promise_ = nullptr;
};

template <typename T = void>
struct process: process_base {
    using process_base::process_base;

    struct promise_type: promise_base {
        template <typename ...Args>
        promise_type(Args && ...): promise_base() {
            coro_ = std::coroutine_handle<promise_type>::from_promise(*this);
        };

        std::optional<T> result;

        process get_return_object() {
            return process(coro_, this);
        }

        void return_value(const T& t) {
            result = t;
            do_return();
        }

        void return_value(T&& t) {
            result = std::move(t);
            do_return();
        }
    };

    T &on_resume() {
        return result();
    }

    auto &start(environment &env) {
        process_base::start(env);
        return *this;
    }

    auto &start(environment *env) {
        process_base::start(env);
        return *this;
    }

    auto &priority(priority_type priority) {
        process_base::priority(priority);
        return *this;
    }

    auto &latency(time_type latency) {
        process_base::latency(latency);
        return *this;
    }

    T &result() {
        auto promise = (promise_type *) this_promise();
        #ifdef CXXDES_SAFE
        if (!promise->result) {
            throw std::runtime_error("no return value from the process!");
        }
        #endif
        return *(promise->result);
    }
};

template <>
struct process<void>: process_base {
    using process_base::process_base;

    struct promise_type: promise_base {
        template <typename ...Args>
        promise_type(Args && ...): promise_base() {
            coro_ = std::coroutine_handle<promise_type>::from_promise(*this);
        };

        process get_return_object() {
            return process(coro_, this);
        }

        void return_void() {
            do_return();
        }
    };

    void on_resume() {
    }

    auto &start(environment &env) {
        process_base::start(env);
        return *this;
    }

    auto &start(environment *env) {
        process_base::start(env);
        return *this;
    }

    auto &priority(priority_type priority) {
        process_base::priority(priority);
        return *this;
    }

    auto &latency(time_type latency) {
        process_base::latency(latency);
        return *this;
    }
};

template <typename T>
concept awaitable = requires(T t, promise_base *promise, coro_handle coro) {
    { t.on_suspend(promise, coro) } -> std::convertible_to<event *>;
    { t.on_resume() };
};

template <awaitable T>
struct wrap_awaitable: T {
    using awaitable_return_value = decltype(std::declval<T>().on_resume());

    template <typename ...U>
    wrap_awaitable(promise_base *promise, U && ...u):
        promise_{promise},
        T{std::forward<U>(u)...} {
    }

    bool await_ready() {
        return false;
    }

    bool await_suspend(coro_handle coro) {
        auto e = T::on_suspend(promise_, coro);
        #ifdef CXXDES_SAFE
        if (e->handler) {
            throw std::runtime_error("a resuming event is required!");
        }
        #endif
        return true;
    }

    auto &await_resume() requires (not (std::is_same_v<awaitable_return_value, void>)) {
        return T::on_resume();
    }

    void await_resume() requires (std::is_same_v<awaitable_return_value, void>) {
        T::on_resume();
    }

private:
    promise_base *promise_ = nullptr;
};

template <typename T>
inline auto promise_base::await_transform(T &&t) {
    return wrap_awaitable<std::unwrap_ref_decay_t<T>>(this, std::forward<T>(t));
}

inline auto promise_base::await_transform(get_env_type) {
    struct awaitable_type {
        environment *env;

        bool await_ready() {
            return true;
        }

        void await_suspend(coro_handle coro) {
        }

        auto await_resume() {
            return env;
        }
    };

    return awaitable_type{env};
}


template <typename T>
struct is_process_impl: std::false_type {  };

template <typename T>
struct is_process_impl<process<T>>: std::true_type {  };

template <typename T>
struct process_return_type_impl {  };

template <typename T>
struct process_return_type_impl<process<T>> {
    using return_type = T;
};

template <typename T>
constexpr auto is_process = is_process_impl<T>::value;

template <typename T>
using process_return_type = typename process_return_type_impl<T>::return_type;

template <typename T, typename R>
concept process_returning = is_process<T> && std::is_convertible_v<process_return_type<T>, R>;

} /* namespace ns_process */
} /* namespace detail */

using detail::ns_process::promise_base;
using detail::ns_process::process;
using detail::ns_process::awaitable;

using detail::ns_process::is_process;
using detail::ns_process::process_return_type;
using detail::ns_process::process_returning;

constexpr detail::ns_process::get_env_type get_env;

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_PROCESS_HPP_INCLUDED */
