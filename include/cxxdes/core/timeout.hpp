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

        if (priority == priority_consts::inherit) {
            priority = priority;
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

    auto time_unit() const noexcept {
        return env_->time_unit();
    }

    auto time_precision() const noexcept {
        return env_->time_precision();
    }

protected:
    environment *env_ = nullptr;
    token *tkn_ = nullptr;

    priority_type priority_;

    auto derived() noexcept -> auto & {
        return static_cast<Derived &>(*this);
    }
};

template <cxxdes::time_ops::detail::node Node>
auto timeout(Node &&node, priority_type priority = priority_consts::inherit) {
    struct result: timeout_base<result> {
        // for some reason Apple Clang do not see time_precision() alone
        using base = timeout_base<result>;

        std::remove_reference_t<Node> node;

        auto latency() {
            return node.count(base::time_precision());
        }
    };

    return result{ { priority }, std::forward<Node>(node) };
}

template <cxxdes::time_ops::detail::scalar Scalar>
auto timeout(Scalar &&scalar, priority_type priority = priority_consts::inherit) {
    struct result: timeout_base<result> {
        using base = timeout_base<result>;

        std::remove_reference_t<Scalar> scalar;

        auto latency() {
            using time_ops::operator*;
            return (scalar * base::time_unit()).count(base::time_precision());
        }
    };

    return result{ { priority }, std::forward<Scalar>(scalar) };
}

template <std::integral Integer>
auto delay(Integer integer, priority_type priority = priority_consts::inherit) {
    struct result: timeout_base<result> {
        using base = timeout_base<result>;

        Integer integer;

        auto latency() {
            return integer;
        }
    };

    return result{ { priority }, integer };
}

inline auto yield() {
    return delay(0);
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_TIMEOUT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_TIMEOUT_HPP_INCLUDED */
