/**
 * @file simulation.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Simulation convenience class.
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

/**
 * @brief CRTP wrapper for simulations with a `co_main()` process.
 *
 * `simulation<Derived>` owns an `environment` and binds `Derived::co_main()` on
 * the first call to `run()` or `run_for()`. The derived type is expected to
 * provide `coroutine<> co_main()`.
 *
 * @tparam Derived Simulation type inheriting from this base.
 */
template <typename Derived>
struct simulation {
    /** @brief Returns the current environment timestamp in simulation ticks. */
    auto now() const noexcept {
        return env.now();
    }

    /** @brief Returns the current environment time as a physical quantity. */
    auto t() const noexcept {
        return env.t();
    }

    /** @brief Returns the current environment time converted to seconds. */
    auto now_seconds() const noexcept {
        return env.now_seconds();
    }

    /** @brief Environment owned by this simulation instance. */
    environment env;

    /** @brief Binds `co_main()` once and runs until no events remain. */
    void run() {
        bind();
        env.run();
    }

    /** @brief Binds `co_main()` once and runs for the requested duration. */
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
    /** @brief Binds `Derived::co_main()` to `env` if it has not been bound yet. */
    void bind() {
        if (!bound_) {
            env.bind(derived().co_main());
            bound_ = true;
        }
    }
};

/** @brief Defines a simulation type deriving from `cxxdes::core::simulation`. */
#define CXXDES_SIMULATION(name) struct name : cxxdes::core::simulation < name >

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_SIMULATION_HPP_INCLUDED */
