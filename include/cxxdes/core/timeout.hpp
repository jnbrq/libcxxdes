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

#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

struct timeout {
    constexpr timeout(time_type latency, priority_type priority = priority_consts::inherit):
        latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro_handle current_coro) {
        tkn_ = new token(env_->now() + latency_, priority_, current_coro);
        env_->schedule_token(tkn_);
    }

    token *await_token() const noexcept {
        return tkn_;
    }

    void await_resume() {
    }
private:
    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_type latency_;
    priority_type priority_;
};

constexpr auto yield = timeout{0};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_TIMEOUT_HPP_INCLUDED */
