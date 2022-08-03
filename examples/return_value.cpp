#define CXXDES_LIBRARY_DEBUG

#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(return_value_example) {
    process<int> test() {
        co_await timeout(10);
        co_return 5;
    }

    process<> co_main() {
        auto result = co_await test();
        fmt::print("now = {}, value = {}\n", now(), result);
    }
};

int main() {
    return_value_example{}.run();
    return 0;
}
