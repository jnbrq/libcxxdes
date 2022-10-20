#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <iostream>
#include <string>

using namespace cxxdes::core;

CXXDES_SIMULATION(mutex_example)
{
    cxxdes::sync::mutex m;

    process<> p(int idx, time_integral t) {
        co_with(m) {
            fmt::print("idx = {}, now = {}\n", idx, now());
            co_await delay(t);
            fmt::print("idx = {}, now = {}\n", idx, now());
        };
    }

    process<> co_main() {
        using namespace cxxdes::core::time_ops;
        co_await all_of(
            p(1, 5).priority(5),
            p(2, 5).priority(3),
            p(3, 5).priority(2),
            p(4, 5).priority(4),
            p(5, 5).priority(1)
        );
    }
};

int main() {
    mutex_example{}.run();
    return 0;
}
