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
namespace ns_timeout {

struct timeout {
    timeout(time_type latency): latency{latency} {  }

    time_type latency;

    event *on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle) {
        auto evt = new event(promise->env->now() + latency, 1000, coroutine_handle);
        promise->env->append_event(evt);
        return evt;
    }

    void on_resume() {  }
};

} // namespace ns_timeout
} // namespace detail


[[nodiscard("expected usage: co_await timeout(latency)")]]
inline auto timeout(time_type latency) {
    return detail::ns_timeout::timeout(latency);
}

} // namespace cxx_des

#endif // CXX_DES_AWAITABLES_HPP_INCLUDED
