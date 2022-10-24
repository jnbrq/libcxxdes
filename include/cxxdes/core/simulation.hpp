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

#include <type_traits>
#include <cxxdes/sync/event.hpp>
#include <cxxdes/core/core.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

template <typename Derived>
struct simulation {
    simulation(environment &env): env_{env} {
        env_.bind(derived().co_main());
    }

    auto now() const noexcept {
        return env_.now();
    }

    auto t() const noexcept {
        return env_.t();
    }

    auto now_seconds() const noexcept {
        return env_.now_seconds();
    }

    auto &env() noexcept {
        return env_;
    }

    auto const& env() const noexcept {
        return env_;
    }

    template <typename ...Args>
    static void run(Args && ...args) {
        environment env;
        Derived s(env, std::forward<Args>(args)...);
        (void) s;
        env.run();
    }

    template <typename T, typename ...Args>
    static void run_for(T &&t, Args && ...args) {
        environment env;
        Derived s(env, std::forward<Args>(args)...);
        (void) s;
        env.run_for(std::forward<T>(t));
    }
    
private:
    auto derived() noexcept -> auto& {
        return static_cast<Derived &>(*this);
    }

    auto derived() const noexcept -> auto const& {
        return static_cast<Derived &>(*this);
    }

protected:
    environment &env_;
};

#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
