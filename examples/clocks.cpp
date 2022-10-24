#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <string>

using namespace cxxdes::core;

environment env;

struct clock_class {
    time_integral period;
    std::string name;

    coroutine<> operator()() {
        while (true) {
            fmt::print("{}: now = {}\n", name, env.now());
            co_await timeout(period);
        }
    }
};

coroutine<> clock(time_integral period, std::string name) {
    while (true) {
        fmt::print("{}: now = {}\n", name, env.now());
        co_await timeout(period);
    }
}

int main() {
    env.bind(clock(2, "clock 1"));
    env.bind(clock(3, "clock 2"));

    auto clock_instance = clock_class{7, "clock 3"};
    env.bind(clock_instance());

    env.run_for(20);
}
