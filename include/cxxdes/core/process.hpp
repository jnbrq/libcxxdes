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

#include <cxxdes/misc/utils.hpp>
#include <cxxdes/misc/reference_counted.hpp>
#include <cxxdes/core/environment.hpp>
#include <cxxdes/core/awaitable.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_PROCESS
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {


#ifdef CXXDES_INTERRUPTABLE

namespace detail {

template <awaitable A>
struct interruptable {
    A a;

    template <typename T>
    explicit interruptable(T &&t): a{std::forward<T>(t)} {
    }

    void await_bind(environment *env, priority_type priority) {
        env_ = env;
        a.await_bind(env, priority);
    }
    
    bool await_ready() {
        return a.await_ready();
    }
    
    void await_suspend(coro_handle current_coro) {
        a.await_suspend(current_coro);
    }

    token *await_token() {
        return a.await_token();
    }

    auto await_resume() {
        if (env_->get_coro_manager().stopped())
            throw interrupted_exception{};
        return a.await_resume();
    }

private:
    environment *env_ = nullptr;
};

} /* namespace detail */

#endif

struct this_process {
    struct get_priority {  };
    struct set_priority {
        priority_type priority;
    };

    struct get_environment {  };
};

template <typename T>
struct await_transform_extender;

template <typename ReturnType = void>
struct process {
private:
    using return_container_type = std::conditional_t<
        std::is_same_v<ReturnType, void>,
        cxxdes::util::empty_type,
        std::optional<ReturnType>
    >;

    struct process_info;
public:
    explicit process(memory::ptr<process_info> pinfo = nullptr): pinfo_{pinfo} {
    }

    void await_bind(environment *env, priority_type priority = priority_consts::zero) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        if (pinfo_->env) {
            if (pinfo_->env != env)
                throw std::runtime_error("cannot bind an already bound to a different environment!");
            
            if (!pinfo_->start_token)
                // already started
                return ;
        }
        
        pinfo_->env = env;

        auto start_token = pinfo_->start_token;
        pinfo_->start_token = nullptr; // env will own the start token

        if (start_token->priority == priority_consts::inherit)
            start_token->priority = priority;
        pinfo_->priority = start_token->priority;
        start_token->time += env->now();
        env->schedule_token(start_token);
#ifdef CXXDES_INTERRUPTABLE
        pinfo_->add_coro();
#endif
    }

    bool await_ready() const noexcept {
        return pinfo_->complete;
    }

    void await_suspend(coro_handle current_coro) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        completion_token_ = new token{ret_latency_, ret_priority_, current_coro};
        if (completion_token_->priority == priority_consts::inherit)
            completion_token_->priority = pinfo_->priority;
        pinfo_->completion_tokens.push_back(completion_token_);
    }

    token *await_token() const noexcept {
        return completion_token_;
    }

    ReturnType await_resume() const {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!pinfo_->return_container)
                throw std::runtime_error("no return value from the process<T> [T != void]!");
            
            // NOTE ReturnType should be copy constructable
            return (*pinfo_->return_container);
        }
    }

    auto &priority(priority_type priority) {
        if (!pinfo_->start_token)
            throw std::runtime_error("cannot change the priority of a started process");
        
        pinfo_->start_token->priority = priority;
        return *this;
    }

    auto &latency(time_integral latency) {
        if (!pinfo_->start_token)
            throw std::runtime_error("cannot change the latency of a started process");
        
        pinfo_->start_token->time = latency;
        return *this;
    }

    auto &return_priority(priority_type priority) noexcept {
        ret_priority_ = priority;
        return *this;
    }

    auto &return_latency(time_integral latency) noexcept {
        ret_latency_ = latency;
        return *this;
    }

    bool is_complete() const noexcept {
        return pinfo_->complete;
    }

    auto return_value() const {
        // TODO make presence of this function conditional on ReturnType
        return await_resume();
    }

    bool is_valid() const noexcept {
        return pinfo_;
    }

    ~process() {
        if (pinfo_ && pinfo_->ref_count() == 2) {
            if (pinfo_->env == nullptr) {
                // not yet started
                // last process<> reference
                // no way to be destroyed

                pinfo_->coro.destroy();
            }
        }
    }

private:
    // we need these mixins, because return_value and return_void cannot coexist.
    // even with concepts, it does not work.

    template <typename Derived>
    struct return_value_mixin {
        template <typename T>
        void return_value(T &&t) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            static_cast<Derived *>(this)->set_return_value(std::forward<T>(t));
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

    struct process_info: memory::reference_counted_base<process_info> {
        process_info() {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            completion_tokens.reserve(2);
        }

        ~process_info() {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            if (start_token) delete start_token;            
            for (auto completion_token: completion_tokens)
                delete completion_token;
        }

        environment *env = nullptr;
        token *start_token = nullptr;
        std::vector<token *> completion_tokens;
        priority_type priority = 0;
        bool complete = false;
        coro_handle coro;

#ifdef CXXDES_INTERRUPTABLE
        void add_coro() {
            if (env && coro) {
                env->get_coro_manager().add_coro(coro);
            }
        }

        void remove_coro() {
            if (env && coro) {
                env->get_coro_manager().remove_coro(coro);
            }
        }
#endif

        // this should come last, so that its padding is not reused
        // see: https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
        [[no_unique_address]]
        return_container_type return_container;
    };
    
public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        > {
        memory::ptr<process_info> pinfo = nullptr;

        template <typename ...Args>
        promise_type(Args && ...) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            pinfo = new process_info;
            auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
            pinfo->coro = coro;
            pinfo->start_token = new token{0, priority_consts::inherit, coro};
        }

        process get_return_object() {
            return process(pinfo);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }
        auto unhandled_exception() {
#ifdef CXXDES_INTERRUPTABLE
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (interrupted_exception & /* ex */) {
                // it is fine
            }
            catch (...) {
                std::rethrow_exception(std::current_exception());
            }
#else
            std::rethrow_exception(std::current_exception());
#endif
        }

#ifdef CXXDES_INTERRUPTABLE
        template <awaitable A>
        auto await_transform(A &&a) const {
            // co_await (A{});
            // A{} is alive throughout the co_await expression
            // therefore, it is safe to bind a reference to it

            auto result = detail::interruptable<std::remove_cvref_t<A>>(std::forward<A>(a));
            result.await_bind(pinfo->env, pinfo->priority);
            return result;
        }
#else
        template <awaitable A>
        auto &&await_transform(A &&a) const {
            // co_await (A{});
            // A{} is alive throughout the co_await expression
            // therefore, it is safe to return an rvalue-reference to it

            a.await_bind(pinfo->env, pinfo->priority);
            return std::forward<A>(a);
        }
#endif

        // for co_with (experimental)
        #ifdef CXXDES_CO_WITH

        template <awaitable A>
        auto &&yield_value(A &&a) {
            return await_transform(std::forward<A>(a));
        }

        #endif // CXXDES_CO_WITH

        // BEGIN implementation of the this_process interface

        auto await_transform(this_process::get_priority) const {
            return immediately_returning_awaitable{pinfo->priority};
        }

        auto await_transform(this_process::set_priority x) {
            pinfo->priority = x.priority;
            return std::suspend_never{};
        }

        auto await_transform(this_process::get_environment) const {
            return immediately_returning_awaitable{pinfo->env};
        }

        // END implementation of the this_process interface

        template <typename T>
        auto await_transform(await_transform_extender<T> const &a) {
            return a.await_transform(*this);
        }

        template <typename T>
        void set_return_value(T &&t) {
            pinfo->return_container = std::forward<T>(t);
        }

        void do_return() {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            for (auto completion_token: pinfo->completion_tokens) {
                completion_token->time += pinfo->env->now();
                pinfo->env->schedule_token(completion_token);
            }

            pinfo->completion_tokens.clear();
            pinfo->complete = true;
#ifdef CXXDES_INTERRUPTABLE
            pinfo->remove_coro();
#endif
        }

        ~promise_type() {
            CXXDES_DEBUG_MEMBER_FUNCTION;
        }
    };

private:
    memory::ptr<process_info> pinfo_ = nullptr;
    token *completion_token_ = nullptr;

    time_integral ret_latency_ = 0;
    priority_type ret_priority_ = priority_consts::inherit;
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

#ifdef CXXDES_UNDER_PROCESS

namespace detail {

struct under_helper {  };

}

template <typename F>
auto operator+(detail::under_helper, F f) {
    return f();
}

#define _Process(...) detail::under_helper{} + [](__VA_ARGS__) -> process<void>

#endif

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_PROCESS
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_PROCESS_HPP_INCLUDED */
