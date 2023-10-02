#include <vector>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

CXXDES_SIMULATION(test) {
    test(std::size_t id): test_id{id} {
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
    test{0}.run_for(100);
    test{1}.run_for(100);
    test{2}.run_for(100);
    return 0;
}
