#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(queue_example) {
    sync::semaphore q;
    
    process<> p1() {
        co_await q.down();
        fmt::print("p1: now = {}\n", now());
    }

    process<> p2() {
        co_await timeout(5);
        co_await q.up();
    }

    process<> co_main() {
        co_await (p1() && p2());
    }
};

int main() {
    queue_example{}.run();
    return 0;
}
