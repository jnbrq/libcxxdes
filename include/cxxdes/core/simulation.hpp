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

namespace cxxdes {
namespace core {

template <typename Derived>
struct simulation {
    auto now() const noexcept {
        return env.now();
    }

    auto t() const noexcept {
        return env.t();
    }

    auto now_seconds() const noexcept {
        return env.now_seconds();
    }

    environment env;

    void run() {
        bind();
        env.run();
    }

    template <typename T>
    void run_for(T &&t) {
        bind();
        env.run_for(std::forward<T>(t));
    }
    
private:
    auto derived() noexcept -> auto& {
        return static_cast<Derived &>(*this);
    }

    auto derived() const noexcept -> auto const& {
        return static_cast<Derived &>(*this);
    }

    bool bound_ = false;

protected:
    void bind() {
        if (!bound_) {
            env.bind(derived().co_main());
            bound_ = true;
        }
    }
};

#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
