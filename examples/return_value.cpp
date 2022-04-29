#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(return_value_example) {
    process<int> test() {
        co_await timeout(10);
        co_return 5;
    }

    process<> co_main() {
        // test 1
        auto result = co_await test();
        fmt::print("now = {}, value = {}\n", now(), result);

        // test 2
        auto h = test();
        h.start(env);
        co_await timeout(11);
        fmt::print("now = {}, done = {}, value = {}\n", now(), h.done(), h.result());
    }
};

int main() {
    return_value_example{}.run();
    return 0;
}
