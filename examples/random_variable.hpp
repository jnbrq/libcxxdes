/**
 * @file random_variable.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Simple random variables.
 * @date 2022-08-03
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef RANDOM_VARIABLE_HPP_INCLUDED
#define RANDOM_VARIABLE_HPP_INCLUDED

#include <random>
#include <chrono>

template <typename Distribution, typename Engine = std::mt19937_64>
struct random_variable {
    template <typename Seed, typename ...Args>
    random_variable(Seed &&seed, Args && ...args):
        e_(std::forward<Seed>(seed)), d_(std::forward<Args>(args)...) {
    }

    auto operator()() {
        return d_(e_);
    }

private:
    Distribution d_;
    Engine e_;
};

using exponential_rv = random_variable<std::exponential_distribution<>>;

inline auto rand_seed() {
    static std::random_device rd;
    return rd();
    // or, you can use chrono (worse)
    // return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

#endif /* RANDOM_VARIABLE_HPP_INCLUDED */
