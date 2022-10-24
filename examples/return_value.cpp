#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(return_value_example) {
    coroutine<int> foo() {
        co_return 4;
    }

    coroutine<int> bar(coroutine<int> &&other) {
        auto r = co_await other;
        std::cout << r << "\n";
        co_return r;
    }
    coroutine<int> test() {
        co_await timeout(10);
        co_return 5;
    }

    coroutine<> co_main() {
        std::cout << co_await bar(foo()) << "!\n";
        auto result = co_await test();
        fmt::print("now = {}, value = {}\n", now(), result);
    }
};

int main() {
    return_value_example::run();
    return 0;
}
