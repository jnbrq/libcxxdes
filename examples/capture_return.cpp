#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <iostream>

using namespace cxxdes::core;


CXXDES_SIMULATION(example) {
    using simulation::simulation;
    
    coroutine<int> f(int x) {
        fmt::print("x = {}, priority = {}\n", x, (co_await this_coroutine())->priority());
        std::cout.flush();
        co_return x;
    }

    coroutine<> co_main() {
        int x;

        co_await (f(90) >> x);
        fmt::print("return value = {}\n", x);

        co_await all_of(f(66) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((f(50) >> x).priority(-5), f(80) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((f(50) >> x).priority(5), f(80) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        // x = 50 right now
        bool done = false;
        co_await any_of((f(90) >> x, flag_done(done)).latency(2), timeout(1));
        fmt::print("return value = {}, done = {}\n", x, done);
        co_await timeout(1, 2);
        fmt::print("return value = {}, done = {}\n", x, done);
    }
};

int main() {
    example::run();
    return 0;
}
