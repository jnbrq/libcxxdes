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
    void start_awaitable(A &&a) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        std::forward<A>(a).await_bind(&env);
        std::forward<A>(a).await_ready();
        std::forward<A>(a).await_suspend(nullptr);
    }

    void start_main() {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        start_awaitable(static_cast<Derived *>(this)->co_main());
    }

    auto now() const {
        return env.now();
    }

    void run() {
        start_main();
        while (env.step()) ;
    }

    void run_until(time_type t) {
        start_main();
        while (now() <= t && env.step());
    }

    void run_for(time_type t) {
        run_until(now() + t);
    }
};

#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
