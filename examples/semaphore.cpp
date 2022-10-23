#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(queue_example) {
    cxxdes::sync::semaphore<> q{1};
    
    coroutine<> p1() {
        co_await q.down();
        co_await q.down();
        co_await q.down();
        fmt::print("p1: now = {}\n", now());
    }

    coroutine<> p2() {
        co_await timeout(5);
        co_await q.up();
    }

    coroutine<> p3() {
        co_await timeout(10);
        co_await q.up();
    }

    coroutine<> co_main() {
        co_await (p1() && p2() && p3());
    }
};

int main() {
    queue_example{}.run();
    return 0;
}
