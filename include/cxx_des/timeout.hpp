/**
 * @file timeout.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Timeout object.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_TIMEOUT_HPP_INCLUDED
#define CXX_DES_TIMEOUT_HPP_INCLUDED

#include "process.hpp"
#include "environment.hpp"

namespace cxx_des {
namespace detail {

struct timeout_base {
    time_type latency;

    void on_suspend(environment *env, std::coroutine_handle<> coroutine_handle) {
        env->append_event(new event{env->now() + latency, 1000, coroutine_handle});
    }

    void on_resume() {}
};

};

using timeout = awaitable<detail::timeout_base>;

}

#endif // CXX_DES_AWAITABLES_HPP_INCLUDED
