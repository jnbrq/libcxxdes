#include <fmt/core.h>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

CXXDES_SIMULATION(async_example) {
    using simulation::simulation;

    coroutine<int> p1() {
        co_await delay(10);
        co_return 1;
    }

    coroutine<int> p2() {
        co_await delay(5);
        co_return 1;
    }

    coroutine<> co_main() {
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

        {
            coroutine<int> p = p1();
            co_await p;

            auto f1 = [](coroutine<int> p) -> coroutine<void> {
                auto env = co_await this_environment();
                co_await p;
                fmt::print("f1: now = {}\n", env->now());
            }(p);

            auto f2 = [](coroutine<int> p) -> coroutine<void> {
                auto env = co_await this_environment();
                co_await p;
                fmt::print("f2: now = {}\n", env->now());
            }(p);

            co_await (f1 && f2);
            fmt::print("after f1 && f2: now = {}\n", now());
            co_await p;
            fmt::print("after p: now = {} ret = {}\n", now(), co_await p);
        }

        {
            coroutine<int> p = p1();
            co_await (p || delay(1)); // p is still running

            auto t1 = now();
            co_await p;
            fmt::print("time passed: {}\n", now() - t1);
        }
    }
};

int main() {
    async_example{}.run();
    return 0;
}
