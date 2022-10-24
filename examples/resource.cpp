#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(resource_example) {
    cxxdes::sync::resource resource{3};

    coroutine<> p(int id, time_integral duration) {
        fmt::print("coroutine #{}: try acquire the resource. @{}\n", id, now());
        auto handle = co_await resource.acquire();
        fmt::print("coroutine #{}: acquired, start using the resource. @{}\n", id, now());
        co_await timeout(duration);
        fmt::print("coroutine #{}: release. @{}\n", id, now());
        co_await handle.release();
        fmt::print("coroutine #{}: done. @{}\n", id, now());
    }

    coroutine<> co_main() {
        co_await all_of(
            p(0, 4),
            p(1, 10),
            p(2, 2),
            p(3, 10).priority(10000 /* make it last to acquire the resource */)
        );
        // in the end, coroutine #3 finishes at 12.
        co_return ;
    }
};

int main() {
    resource_example::run();
    return 0;
}
