#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(queue_example) {
    sync::queue<int> q;
    
    process<> p1() {
        auto r = co_await q.pop();
        fmt::print("p1: r = {}, now = {}\n", r, now());
    }

    process<> p2() {
        co_await timeout(5);
        co_await q.put(5);
    }

    process<> co_main() {
        co_await (p1() && p2());
    }
};

int main() {
    queue_example{}.run();
    return 0;
}
