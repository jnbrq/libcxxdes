/**
 * @file simulation.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Simulation comodity class.
 * @date 2022-04-17
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_SIMULATION_HPP_INCLUDED
#define CXXDES_CORE_SIMULATION_HPP_INCLUDED

#include <cxxdes/core/environment.hpp>
#include <cxxdes/core/awaitable.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

template <typename Derived>
struct simulation {
    environment env;

    template <awaitable A>
    void start_awaitable(A a) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        a.await_bind(&env);
        a.await_ready();
        a.await_suspend(nullptr);
    }

    auto now() const {
        return env.now();
    }

    auto t() const {
        return env.t();
    }

    auto now_seconds() const {
        return env.now_seconds();
    }

    auto run() {
        auto p = static_cast<Derived *>(this)->co_main();
        start_awaitable(p);
        while (env.step());
        return p.await_resume();
    }

    void run_until(time_integral t) {
        auto p = static_cast<Derived *>(this)->co_main();
        start_awaitable(p);
        while (now() <= t && env.step());
    }

    void run_for(time_integral t) {
        run_until(now() + t);
    }
private:
    auto derived() noexcept -> auto & {
        return static_cast<Derived &>(*this);
    }
};

#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
