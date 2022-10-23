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
    clock(2, "clock 1").await_bind(&env);
    clock(3, "clock 2").await_bind(&env);

    auto clock_instance = clock_class{7, "clock 3"};
    clock_instance().await_bind(&env);

    while (env.step() && env.now() < 10) ;
}
