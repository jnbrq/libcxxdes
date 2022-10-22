/**
 * @file timeout.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Timeout object.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_TIMEOUT_HPP_INCLUDED
#define CXXDES_CORE_TIMEOUT_HPP_INCLUDED

#include <cxxdes/core/core.hpp>
#include <cxxdes/misc/time.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_TIMEOUT
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

template <typename Derived>
struct timeout_base {
    constexpr timeout_base(priority_type priority = priority_consts::inherit): priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(process_handle phandle) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        auto latency = derived().latency();
        tkn_ = new(env_->memres()) token(env_->now() + latency, priority_, phandle);
        env_->schedule_token(tkn_);
    }

    token *await_token() const noexcept {
        return tkn_;
    }

    void await_resume(no_return_value_tag = {}) const noexcept {
        CXXDES_DEBUG_MEMBER_FUNCTION;
    }

    auto &env() const noexcept {
        return *env_;
    }

protected:
    environment *env_ = nullptr;
    token *tkn_ = nullptr;

    priority_type priority_;

    auto derived() noexcept -> auto & {
        return static_cast<Derived &>(*this);
    }
};

template <typename T>
[[nodiscard("expected usage: co_await timeout(t)")]]
constexpr auto timeout(T &&t, priority_type priority = priority_consts::inherit) noexcept {
    struct result: timeout_base<result> {
        using base = timeout_base<result>;

        T t;

        auto latency() const noexcept {
            return base::env().real_to_sim(t);
        }
    };

    return result{ { priority }, std::forward<T>(t) };
}

struct instant_type: timeout_base<instant_type> {
    const time_integral t = 0;

    bool await_ready() {
        return env().now() >= t;
    }

    auto latency() const noexcept {
        return t - env().now();
    }
};

template <typename T>
constexpr auto lazy_timeout(T &&t, priority_type priority = priority_consts::inherit) {
    struct result_type {
        // TODO for both timeout() and instant(), should they copy always?
        T t; // should we use std::remove_cvref_t?
        priority_type priority;
        time_integral tsim = 0;

        void await_bind(environment *env, priority_type) noexcept {
            tsim = env->now() + env->real_to_sim(t);
        }

        bool await_ready() const noexcept {
            return true;
        }

        void await_suspend(process_handle) const noexcept {  }

        token *await_token() const noexcept { return nullptr; }

        auto await_resume() {
            return instant_type{ { priority }, tsim };
        }

        void await_resume(no_return_value_tag) {  }
    };

    return result_type{std::forward<T>(t), priority};
}

struct delay_type: timeout_base<delay_type> {
    using base = timeout_base<delay_type>;

    time_integral integer;

    auto latency() const noexcept {
        return integer;
    }
};

template <std::integral Integer>
constexpr auto delay(Integer delay, priority_type priority = priority_consts::inherit) noexcept {
    return delay_type{ { priority }, static_cast<time_integral>(delay) };
}

constexpr auto yield() noexcept {
    return delay(0);
}

template <typename T>
inline auto environment::timeout(T &&t) const noexcept {
    return delay(real_to_sim(t));
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_TIMEOUT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_TIMEOUT_HPP_INCLUDED */
