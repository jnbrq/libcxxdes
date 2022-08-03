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

namespace cxxdes {
namespace core {

template <typename Derived>
struct simulation {
    environment env;

    void start_main() {
        ((Derived &) *this).co_main().await_bind(&env);
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

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
