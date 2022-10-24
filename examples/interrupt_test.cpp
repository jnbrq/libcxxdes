#include <vector>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

CXXDES_SIMULATION(test) {
    test(environment &env, std::size_t id):
        simulation(env), test_id{id} {
    }

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
    test::run_for(100, 0);
    test::run_for(100, 1);
    test::run_for(100, 2);
    return 0;
}
