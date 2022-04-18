#include <cxxdes/cxxdes.hpp>
#include <iostream>

using namespace cxxdes;

CXXDES_SIMULATION(queue_example) {
    sync::queue<int> q;
    
    process<> p1() {
        auto r = co_await q.pop();
        std::cout << "p1, r = " << r << ", now = " << now() << "\n";
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
