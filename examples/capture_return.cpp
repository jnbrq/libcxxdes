#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <iostream>

using namespace cxxdes;


CXXDES_SIMULATION(example) {
    process<int> f(int x) {
        fmt::print("x = {}, priority = {}\n", x, co_await this_process::get_priority());
        std::cout.flush();
        co_return x;
    }

    process<> co_main() {
        int x;

        co_await (f(90) >> x);
        fmt::print("return value = {}\n", x);

        co_await all_of(f(66) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((f(50) >> x).priority(-5), f(80) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((f(50) >> x).priority(5), f(80) >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        // x = 50 right now
        bool done = false;
        co_await any_of((timeout(2), f(90) >> x, flag_done(done)), timeout(1));
        fmt::print("return value = {}, done = {}\n", x, done);
        co_await timeout(1, 2);
        fmt::print("return value = {}, done = {}\n", x, done);
    }
};

int main() {
    example{}.run();
    return 0;
}
