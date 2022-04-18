#include <cxxdes/cxxdes.hpp>
#include <iostream>

using namespace cxxdes;

CXXDES_SIMULATION(any_of_example) {
    process<int> test() {
        co_await timeout(10);
        co_return 5;
    }

    process<> co_main() {
        auto result = co_await test();
        std::cout << "now = " << now() << " value = " << result << std::endl;
    }
};

int main() {
    any_of_example{}.run();
    return 0;
}
