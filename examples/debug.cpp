#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(debug_example) {
    using simulation::simulation;
    
    void print_time(int id) {
        fmt::print("from {}, time = {}\n", id, now());
    }

    coroutine<> p1() {
        co_await timeout(5);
        print_time(1);
        co_await timeout(5);
        print_time(1);
    }

    coroutine<> p2() {
        co_await timeout(2);
        print_time(2);
        co_await timeout(3, -1);
        print_time(2);
    }

    coroutine<> co_main() {
        co_await all_of(
            p1(),
            p2()
        );
    }
};

int main() {
    debug_example{}.run();
    return 0;
}
