#include <cxxdes/cxxdes.hpp>
#include <iostream>
#include <string>

using namespace cxxdes::core;

CXXDES_SIMULATION(mutex_example)
{
    cxxdes::sync::mutex m;

    template <typename A = const char *>
    void print_time(A &&a = "") {
        std::cout << a << " " << now() << std::endl;
    }

    process<> p1() {
        auto h = co_await m.acquire();
        std::cout << "p1 works\n";
        co_await timeout(5);
        co_await h.release();
    }

    process<> p2() {
        auto h = co_await m.acquire();
        std::cout << "p2 works\n";
        co_await timeout(5);
        co_await h.release();
    }

    process<> co_main() {
        co_await (p1() && p2().priority(-10));
    }
};

int main() {
    mutex_example{}.run();
    return 0;
}
