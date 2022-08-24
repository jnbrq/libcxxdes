#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(bug) {
    process<int> test() {
        co_return 10;
    }

    process<> co_main() {
        fmt::print("return value = {}\n", co_await test());
        co_await (test() && timeout(10));
    }
};

int main() {
    bug{}.run();
    return 0;
}
