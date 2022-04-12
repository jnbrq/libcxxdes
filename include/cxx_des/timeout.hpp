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

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> coroutine_handle) {
        auto evt = new event(promise.env->now() + latency, 1000, coroutine_handle);
        promise.env->append_event(evt);
        return evt;
    }

    void on_resume() {  }
};

};

using timeout = awaitable<detail::timeout_base>;

}

#endif // CXX_DES_AWAITABLES_HPP_INCLUDED
