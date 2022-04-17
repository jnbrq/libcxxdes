#include <cxxdes/cxxdes.hpp>
#include <iostream>

using namespace cxxdes;

CXXDES_SIMULATION(any_of_example) {
    process<> co_main() {
        std::cout << "p1.a now " << env.now() << std::endl;
        co_await ((timeout(1000) && timeout(5)) || (timeout(100) && timeout(1)));
        std::cout << "p1.b now " << env.now() << std::endl;
        co_await all_of(timeout(10), timeout(20));
        std::cout << "p1.c now " << env.now() << std::endl;
        co_await any_of(timeout(10), timeout(20));
        std::cout << "p1.d now " << env.now() << std::endl;
    }
};

int main() {
    any_of_example{}.run();
    return 0;
}
