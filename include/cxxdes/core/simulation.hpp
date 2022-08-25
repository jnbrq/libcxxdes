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

    auto now() const noexcept {
        return env.now();
    }

    auto t() const noexcept {
        return env.t();
    }

    auto now_seconds() const noexcept {
        return env.now_seconds();
    }
    
    void run() {
        if (main_process_.is_valid() && main_process_.is_complete())
            return ;

        if (not main_process_.is_valid()) {
            main_process_ = derived().co_main();
            start_awaitable(main_process_);
        }
        
        while (env.step());
    }

    auto &run_until(time_integral t) {
        if (not main_process_.is_valid()) {
            main_process_ = derived().co_main();
            start_awaitable(main_process_);
        }

        if (not main_process_.is_complete())
            while (now() <= t && env.step());
        return *this;
    }

    auto &run_until(time_expr t) {
        run_until(env.real_to_sim(t));
        return *this;
    }

    auto &run_for(time_integral t) {
        run_until(now() + t);
        return *this;
    }

    auto &run_for(time_expr t) {
        run_until(now() + env.real_to_sim(t));
        return *this;
    }

#ifdef CXXDES_INTERRUPTABLE
    void stop() {
        env.stop();
    }
#endif
private:
    template <awaitable A>
    void start_awaitable(A a) {
        CXXDES_DEBUG_MEMBER_FUNCTION;

        a.await_bind(&env, 0);
        a.await_ready();
        a.await_suspend(nullptr);
    }

    auto derived() noexcept -> auto& {
        return static_cast<Derived &>(*this);
    }

    auto derived() const noexcept -> auto const& {
        return static_cast<Derived &>(*this);
    }

    process<void> main_process_;
};

#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_SIMULATION
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
