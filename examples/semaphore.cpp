#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(queue_example) {
    sync::semaphore<> q{1};
    
    process<> p1() {
        co_await q.down();
        co_await q.down();
        co_await q.down();
        fmt::print("p1: now = {}\n", now());
    }

    process<> p2() {
        co_await timeout(5);
        co_await q.up();
    }

    process<> p3() {
        co_await timeout(10);
        co_await q.up();
    }

    process<> co_main() {
        co_await (p1() && p2() && p3());
    }
};

int main() {
    queue_example{}.run();
    return 0;
}
