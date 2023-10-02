#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(instant_test) {
    using simulation::simulation;
    
    coroutine<void> co_main() {
        using namespace cxxdes::core::time_ops;

        {
            auto t = co_await lazy_timeout(10_x);
            co_await t;
            fmt::print("now = {}\n", now());
        }

        {
            auto t = co_await lazy_timeout(10_x);
            co_await timeout(20_x);
            fmt::print("now = {}\n", now());
            co_await t;
            fmt::print("now = {}\n", now());
        }
    }
};

int main() {
    instant_test{}.run();
    return 0;
}
