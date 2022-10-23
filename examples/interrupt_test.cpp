#define CXXDES_INTERRUPTABLE
#define CXXDES_UNDER_coroutine
// #define CXXDES_DEBUG_CORE_coroutine

#include <vector>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

CXXDES_SIMULATION(test) {
    std::size_t test_id = 0;

    static coroutine<> foo() {
        while (true) {
            co_await delay(10);
        }
    }

    static coroutine<> bar() {
        co_await foo();
    }

    coroutine<> co_main() {
        std::vector<coroutine<>> ps {
            _Coroutine() { co_await ((foo() && foo()) || foo()); },
            _Coroutine() { co_await (foo(), foo()); },
            _Coroutine() { co_await bar(); }
        };
        co_await ps[test_id];
    }
};

int main() {
    {
        auto t = test{ .test_id = 0 };
        t.run_for(100);
    }

    {
        auto t = test{ .test_id = 1 };
        t.run_for(100);
    }

    {
        auto t = test{ .test_id = 2 };
        t.run_for(100);
    }
    return 0;
}
