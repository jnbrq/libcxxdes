#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <vector>

using namespace cxxdes::core;

CXXDES_SIMULATION(complicated_example) {
    cxxdes::sync::event evt;

    coroutine<> p0() {
        co_await timeout(5);

        co_return;
    }

    coroutine<> p1() {
        co_await timeout(5);
        co_await evt.wake();

        co_return;
    }

    coroutine<> p2() {
        fmt::print("p2.a now = {}\n", env.now());
        co_await (evt.wait() || timeout(8));
        fmt::print("p2.b now = {}\n", env.now());
        co_await p0();
        fmt::print("p2.c now = {}\n", env.now());
        co_await sequential(timeout(5), timeout(5));
        fmt::print("p2.d now = {}\n", env.now());

        co_return;
    }

    // A very bad example
    coroutine<> p3() {
        co_await sequential(timeout(5), timeout(10), p0());
        fmt::print("p3.a now = {}\n", env.now());

        co_return;
    }

    // Recursion example
    coroutine<> p4(unsigned k) {
        if (k == 0)
            co_return ;
        
        co_await timeout(5);
        fmt::print("p4.a k = {}, now = {}\n", k, env.now());
        co_await p4(k - 1);

        co_return;
    }

    coroutine<> p5() {
        time_integral t = 0;

        fmt::print("Example 5: dynamic versions. now = {}\n", env.now());
        auto to_wait = std::vector{ timeout(5),  timeout(10), timeout(20) };

        t = now();
        co_await all_of.range(to_wait.begin(), to_wait.end());
        fmt::print("p5.delta1: {}\n", env.now() - t);

        t = now();
        co_await any_of.range(to_wait.begin(), to_wait.end());
        fmt::print("p5.delta1: {}\n", env.now() - t);

        co_return;
    }

    coroutine<> co_main() {
        fmt::print("Example 1: p1 and p2 working together. now = {}\n", env.now());
        co_await all_of(p1(), p2());

        fmt::print("Example 2: p3 working alone. now = {}\n", env.now());
        co_await p3();

        fmt::print("Example 3: p4 working alone. now = {}\n", env.now());
        co_await p4(20);

        fmt::print("Example 4: same thing with control flow expressions. now = {}\n", env.now());
        co_await ((p1() && p2()), p3(), p4(20));

        fmt::print("Example 5: dynamic. now = {}\n", env.now());
        co_await p5();

        co_return;
    }
};

int main() {
    complicated_example{}.run();
    return 0;
}
