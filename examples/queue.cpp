#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(queue_example) {
    cxxdes::sync::queue<int> q{1};
    
    coroutine<> p1() {
        auto r1 = co_await q.pop();
        fmt::print("p1: r = {}, now = {}\n", r1, now());
        co_await timeout(7);
        auto r2 = co_await q.pop();
        fmt::print("p1: r = {}, now = {}\n", r2, now());
    }

    coroutine<> p2() {
        co_await timeout(5);
        co_await q.put(1);
        co_await q.put(2);
        fmt::print("p2: now = {}\n", now());
    }

    coroutine<> co_main() {
        co_await (p1() && p2());
    }
};

int main() {
    queue_example{}.run();
    return 0;
}
