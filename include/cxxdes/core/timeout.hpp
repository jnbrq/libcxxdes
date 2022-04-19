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

#include <cxxdes/core/process.hpp>
#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

namespace detail {
namespace ns_timeout {

struct timeout {
    constexpr timeout(time_type latency): latency{latency} {  }

    time_type latency;

    event *on_suspend(promise_base *promise, coro_handle coro) {
        auto evt = new event(promise->env->now() + latency, 1000, coro);
        promise->env->append_event(evt);
        return evt;
    }

    void on_resume() {  }
};

} /* namespace ns_timeout */
} /* namespace detail */


[[nodiscard("expected usage: co_await timeout(latency)")]]
inline auto timeout(time_type latency) {
    return detail::ns_timeout::timeout(latency);
}

[[nodiscard("expected usage: co_await yield")]]
constexpr auto yield = detail::ns_timeout::timeout{0};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_TIMEOUT_HPP_INCLUDED */
