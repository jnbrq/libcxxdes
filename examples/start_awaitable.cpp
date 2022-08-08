#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <optional>

using namespace cxxdes;

CXXDES_SIMULATION(start_awaitable_example) {
    process<int> test() {
        co_return 10;
    }

    process<> co_main() {
        // TODO verify that test() is destroyed when the coro is done
        start_awaitable(test());
        co_await timeout(10);
        fmt::print("done.\n");
    }
};

int main() {
    start_awaitable_example{}.run();
    return 0;
}
