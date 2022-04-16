#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

CXX_DES_SIMULATION(mutex_sim)
{
    mutex m;

    template <typename A = const char *>
    void print_time(A &&a = "") {
        std::cout << a << " " << now() << std::endl;
    }

    process p1() {
        print_time("p1a");
        co_await m.acquire();
        print_time("p1b");
        co_await timeout(10);
        print_time("p1c");
        co_await m.release();
        print_time("p1d");
    }

    process p2() {
        print_time("p2a");
        co_await timeout(5);
        print_time("p2b");
        co_await m.acquire();
        print_time("p2c");
        co_await m.release();
        print_time("p2d");
    }

    process co_main() {
        co_await (p1() && p2());
    }
};

int main() {
    mutex_sim{}.run();
    return 0;
}
