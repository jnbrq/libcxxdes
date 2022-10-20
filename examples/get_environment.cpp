#if 0 // uncomment to print debug messages
#   define CXXDES_DEBUG_CORE_PROCESS
#   define CXXDES_DEBUG_CORE_ENVIRONMENT
#   define CXXDES_DEBUG_CORE_TOKEN
#endif

#include <cxxdes/cxxdes.hpp>

#include <fmt/core.h>

using namespace cxxdes::core;

process<> p1() {
    fmt::print("p1.a\n");
    auto env = co_await this_environment(); // it should not yield control here
    fmt::print("p1.b\n");
    co_await timeout(4);
    fmt::print("p1.c {}\n", env->now());
    co_await env->timeout(4);
    fmt::print("p1.d {}\n", env->now());
}

process<> p2() {
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

    p1().await_bind(&env);
    p2().priority(-10000).await_bind(&env);

    while (env.step()) ;
    return 0;
}
