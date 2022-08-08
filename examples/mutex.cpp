#include <cxxdes/cxxdes.hpp>
#include <iostream>
#include <string>

using namespace cxxdes;

CXXDES_SIMULATION(mutex_example)
{
    sync::mutex m;

    template <typename A = const char *>
    void print_time(A &&a = "") {
        std::cout << a << " " << now() << std::endl;
    }

    process<> p1() {
        co_await m.acquire();
        std::cout << "p1 works\n";
        co_await timeout(5);
        co_await m.release();
    }

    process<> p2() {
        co_await m.acquire();
        std::cout << "p2 works\n";
        co_await timeout(5);
        co_await m.release();
    }

    process<> co_main() {
        co_await (p1() && p2().priority(-10));
    }
};

int main() {
    mutex_example{}.run();
    return 0;
}
