#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(any_of_example) {
    process<> co_main() {
        fmt::print("p1.a now = {}\n", env.now());
        co_await ((timeout(1000) && timeout(5)) || (timeout(100) && timeout(1)));
        fmt::print("p1.b now = {}\n", env.now());
        co_await all_of(timeout(10), timeout(20));
        fmt::print("p1.c now = {}\n", env.now());
        co_await any_of(timeout(10), timeout(20));
        fmt::print("p1.d now = {}\n", env.now());
    }
};

int main() {
    any_of_example{}.run();
    return 0;
}
