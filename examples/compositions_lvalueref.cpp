#include <fmt/core.h>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes;

CXXDES_SIMULATION(async_example) {

    process<int> p1() {
        co_await delay(10);
        co_return 1;
    }

    process<int> p2() {
        co_await delay(5);
        co_return 1;
    }

    process<> co_main() {
        {
            auto a = p1();
            auto b = p2();
            co_await (a && b);
            fmt::print("now = {}\n", now());
        }

        {
            auto a = p1();
            auto b = p2();
            co_await (a || b);
            fmt::print("now = {}\n", now());
        }
    }
};

int main(int argc, char **argv) {
    async_example{}.run();
    return 0;
}
