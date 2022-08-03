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

        co_await (capture_return{f(90)} >> x);
        fmt::print("return value = {}\n", x);

        co_await all_of(capture_return{f(66)} >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((capture_return{f(50)} >> x).priority(-5), capture_return{f(80)} >> x, timeout(5));
        fmt::print("return value = {}\n", x);

        co_await all_of((capture_return{f(50)} >> x).priority(5), capture_return{f(80)} >> x, timeout(5));
        fmt::print("return value = {}\n", x);
    }
};

int main() {
    example{}.run();
    return 0;
}
