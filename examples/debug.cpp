#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(debug_example) {
    void print_time(int id) {
        fmt::print("from {}, time = {}\n", id, env.now());
    }

    process<> p1() {
        co_await timeout(5);
        print_time(1);
        co_await timeout(5);
        print_time(1);
    }

    process<> p2() {
        co_await timeout(2);
        print_time(2);
        co_await timeout(3, -1);
        print_time(2);
    }

    process<> co_main() {
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
