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

#include <cxxdes/time.hpp>
#include <cxxdes/core/environment.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_TIMEOUT
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

template <typename Derived>
struct timeout_base {
    timeout_base(priority_type priority = priority_consts::inherit): priority_{priority} {
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

    void await_suspend(coro_handle current_coro) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        auto latency = derived().latency();
        tkn_ = new token(env_->now() + latency, priority_, current_coro);
        env_->schedule_token(tkn_);
    }

    token *await_token() const noexcept {
        return tkn_;
    }

    void await_resume() const noexcept {
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
auto timeout(T &&t, priority_type priority = priority_consts::inherit) noexcept {
    struct result: timeout_base<result> {
        // for some reason Apple Clang do not see time_precision() alone
        using base = timeout_base<result>;

        T node;

        auto latency() const noexcept {
            return base::env().real_to_sim(node);
        }
    };

    return result{ { priority }, std::forward<T>(t) };
}

struct delay_type: timeout_base<delay_type> {
    using base = timeout_base<delay_type>;

    time_type integer;

    auto latency() const noexcept {
        return integer;
    }
};

template <std::integral Integer>
auto delay(Integer delay, priority_type priority = priority_consts::inherit) noexcept {
    return delay_type{ { priority }, static_cast<time_type>(delay) };
}

inline auto yield() noexcept {
    return delay(0);
}

template <typename T>
auto environment::timeout(T &&t) const noexcept {
    return delay(real_to_sim(t));
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_TIMEOUT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_TIMEOUT_HPP_INCLUDED */
