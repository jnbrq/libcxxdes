/**
 * @file awaitables.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Awaitable objects.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sonmez 2022
 * 
 */

#ifndef CXXDES_AWAITABLES_HPP_INCLUDED
#define CXXDES_AWAITABLES_HPP_INCLUDED

#include "process.hpp"
#include "environment.hpp"

namespace cxx_des {

namespace detail {
    struct timeout_base {
        timeout_base(time_type delta) {
            delta_ = delta;
        }

        void on_suspend(environment *env, std::coroutine_handle<> coroutine_handle) {
            env->append_event(new event{env->now() + delta_, 1000, coroutine_handle});
        }

        void on_resume() {}

    private:
        time_type delta_;
    };
};

using timeout = awaitable<detail::timeout_base>;

}

#endif // CXXDES_AWAITABLES_HPP_INCLUDED
