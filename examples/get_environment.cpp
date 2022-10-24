#if 0 // uncomment to print debug messages
#   define CXXDES_DEBUG_CORE_coroutine
#   define CXXDES_DEBUG_CORE_ENVIRONMENT
#   define CXXDES_DEBUG_CORE_TOKEN
#endif

#include <cxxdes/cxxdes.hpp>

#include <fmt/core.h>

using namespace cxxdes::core;

coroutine<> p1() {
    fmt::print("p1.a\n");
    auto env = co_await this_environment(); // it should not yield control here
    fmt::print("p1.b\n");
    co_await timeout(4);
    fmt::print("p1.c {}\n", env->now());
    co_await env->timeout(4);
    fmt::print("p1.d {}\n", env->now());
}

coroutine<> p2() {
    fmt::print("p2\n");
    co_await yield();

    fmt::print("p2\n");
    co_await yield();

    fmt::print("p2\n");
    co_await yield();

    fmt::print("p2\n");
    co_await yield();

    co_return ;
}

int main() {
    environment env;

    env.bind(p1());
    env.bind(p2().priority(-10000));

    env.run();
    return 0;
}
