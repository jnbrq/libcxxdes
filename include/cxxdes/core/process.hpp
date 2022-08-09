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

#include <cxxdes/core/debug_helpers.hpp>
#include <cxxdes/core/environment.hpp>
#include <cxxdes/core/awaitable.hpp>

namespace cxxdes {
namespace core {


struct this_process {
    struct get_return_latency {  };
    struct set_return_latency {
        time_type latency;
    };

    struct get_return_priority {  };
    struct set_return_priority {
        priority_type priority;
    };
    
    struct get_priority {  };
    struct set_priority {
        priority_type priority;
    };

    struct get_environment {  };
};

struct empty_type {  };

template <typename T>
struct await_transform_extender;

template <typename ReturnType = void>
struct process {
    using return_container_type = std::conditional_t<
        std::is_same_v<ReturnType, void>,
        empty_type,
        std::optional<ReturnType>
    >;

    struct promise_type;

    process(promise_type *this_promise): this_promise_{this_promise} {
        CXXDES_DEBUG_MEMBER_FUNCTION;
    }

    void await_bind(environment *env, priority_type priority = priority_consts::zero) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        if (bound_)
            throw std::runtime_error("cannot bind an already bound process twice");
        bound_ = true;
        this_promise_->bind(env, priority);
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro_handle current_coro) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        completion_tkn_ = new token{0, this_promise_->priority, current_coro};
        if constexpr (not std::is_same_v<ReturnType, void>)
            this_promise_->return_container = &return_container_;
        this_promise_->completion_tkn = completion_tkn_;
    }

    token *await_token() const noexcept {
        return completion_tkn_;
    }

    ReturnType await_resume() {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            // at this point, the promise is already destroyed,
            // however, the coroutine object process<T> is still alive.
            if (!return_container_)
                throw std::runtime_error("no return value from the process<T> [T != void]!");
            return std::move(*return_container_);
        }
    }

    auto &priority(priority_type priority) {
        #ifdef CXXDES_SAFE
        if (bound_)
            throw std::runtime_error("cannot change the priority of a started process");
        #endif
        this_promise_->start_tkn->priority = priority;
        return *this;
    }

    auto &latency(time_type latency) {
        #ifdef CXXDES_SAFE
        if (bound_)
            throw std::runtime_error("cannot change the latency of a started process");
        #endif
        this_promise_->start_tkn->time = latency;
        return *this;
    }

    ~process() {
        CXXDES_DEBUG_MEMBER_FUNCTION;
    }

private:
    // we need these mixins, because return_value and return_void cannot coexist.
    // even with concepts, it does not work.

    template <typename Derived>
    struct return_value_mixin {
        // return value
        return_container_type *return_container = nullptr;

        template <typename T>
        void return_value(T &&t) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            if (return_container)
                (*return_container).emplace(std::forward<T>(t));
            else
                CXXDES_WARNING("{}: return_container == nullptr", __PRETTY_FUNCTION__);
            static_cast<Derived *>(this)->do_return();
        }
    };

    template <typename Derived>
    struct return_void_mixin {
        void return_void() {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            static_cast<Derived *>(this)->do_return();
        }
    };
    
public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        > {
        
        // environment that this process is bound to
        environment *env = nullptr;

        // start token
        token *start_tkn = nullptr;

        // completion token
        token *completion_tkn = nullptr;

        // correspoding coroutine object
        coro_handle this_coro = nullptr;

        // priority to be inherited by the subsequent co_await's
        priority_type priority = 0;

        template <typename ...Args>
        promise_type(Args && ...) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            start_tkn = new token{0, priority_consts::inherit, nullptr};
            this_coro = std::coroutine_handle<promise_type>::from_promise(*this);
        };

        process get_return_object() {
            return process(this);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }
        auto unhandled_exception() { std::rethrow_exception(std::current_exception()); }

        template <awaitable A>
        auto &&await_transform(A &&a) const noexcept {
            // co_await (A{});
            // A{} is alive throughout the co_await expression
            // therefore, it is safe to return an rvalue-reference to it

            a.await_bind(env, priority);
            return std::forward<A>(a);
        }

        // for co_with (experimental)
        #ifdef CXXDES_CO_WITH

        template <awaitable A>
        auto &&yield_value(A &&a) {
            return await_transform(std::forward<A>(a));
        }

        #endif // CXXDES_CO_WITH

        // implementation of the this_process interface

        auto await_transform(this_process::get_return_latency) const {
            if (!completion_tkn) {
                throw std::runtime_error("get_return_latency cannot be called for the main process!");
            }

            return immediately_returning_awaitable<time_type>{completion_tkn->time};
        }

        auto await_transform(this_process::set_return_latency x) {
            if (!completion_tkn) {
                throw std::runtime_error("set_return_latency cannot be called for the main process!");
            }

            completion_tkn->time = x.latency;
            return std::suspend_never{};
        }
        
        auto await_transform(this_process::get_return_priority) const {
            if (!completion_tkn) {
                throw std::runtime_error("get_return_priority cannot be called for the main process!");
            }

            return immediately_returning_awaitable<priority_type>{completion_tkn->priority};
        }

        auto await_transform(this_process::set_return_priority x) {
            if (!completion_tkn) {
                throw std::runtime_error("set_return_priority cannot be called for the main process!");
            }

            completion_tkn->priority = x.priority;
            return std::suspend_never{};
        }

        auto await_transform(this_process::get_priority) const {
            return immediately_returning_awaitable<priority_type>{priority};
        }

        auto await_transform(this_process::set_priority x) {
            priority = x.priority;
            return std::suspend_never{};
        }

        auto await_transform(this_process::get_environment) const {
            return immediately_returning_awaitable<environment *>{env};
        }

        template <typename T>
        auto await_transform(await_transform_extender<T> const &a) {
            return a.await_transform(*this);
        }

        void bind(environment *env, priority_type inherited_priority) {
            this->env = env;
            start_tkn->coro = this_coro;
            if (start_tkn->priority == priority_consts::inherit)
                start_tkn->priority = inherited_priority;
            env->schedule_token(start_tkn);
            priority = start_tkn->priority;
            start_tkn = nullptr;
        }

        void do_return() {
            CXXDES_DEBUG_MEMBER_FUNCTION;
            
            if (completion_tkn) {
                completion_tkn->time += env->now();
                env->schedule_token(completion_tkn);
                completion_tkn = nullptr;
            }
        }

        ~promise_type() {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            if (start_tkn) delete start_tkn;
        }
    };

private:
    promise_type *this_promise_ = nullptr;
    token *completion_tkn_ = nullptr;

    bool bound_ = false;

    // this should come last, so that its padding is not reused
    // see: https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
    [[no_unique_address]]
    return_container_type return_container_;
};

template <typename T>
concept releasable = requires(T t) {
    { t.release() } -> awaitable;
};

template <typename T>
concept acquirable = requires(T t) {
    { t.acquire() } -> awaitable;
    { t.acquire().await_resume() } -> releasable;
};

#ifdef CXXDES_CO_WITH

template <acquirable A, typename F>
process<void> operator+(A &a, F &&f) {
    auto handle = co_await a.acquire();
    co_await std::forward<F>(f)();
    co_await handle.release();
}

#define co_with(x) co_yield (x) + [&]() mutable -> process<void>

#endif // CXXDES_CO_WITH

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_PROCESS_HPP_INCLUDED */
