#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

process<> p1() {
    fmt::print("p1.a\n");
    auto env = co_await get_env; // it should not yield control here
    fmt::print("p1.b\n");
    co_await timeout(4);
    fmt::print("p1.c {}\n", env->now());
}

process<> p2() {
    fmt::print("p2\n");
    co_await yield;

    fmt::print("p2\n");
    co_await yield;

    fmt::print("p2\n");
    co_await yield;

    fmt::print("p2\n");
    co_await yield;

    co_return ;
}

int main() {
    environment env;

    p1().start(env);
    p2().priority(-10000).start(env);

    while (env.step()) ;
    return 0;
}
