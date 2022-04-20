#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(resource_example) {
    sync::resource resource{3};

    process<> p(int id, time_type duration) {
        fmt::print("process #{}: try acquire the resource. @{}\n", id, now());
        auto handle = co_await resource.acquire();
        fmt::print("process #{}: acquired, start using the resource. @{}\n", id, now());
        co_await timeout(duration);
        fmt::print("process #{}: release. @{}\n", id, now());
        co_await handle.release();
        fmt::print("process #{}: done. @{}\n", id, now());
    }

    process<> co_main() {
        co_await all_of(
            p(0, 4),
            p(1, 10),
            p(2, 2),
            p(3, 10).priority(10000 /* make it last to acquire the resource */)
        );
        co_return ;
    }
};

int main() {
    resource_example{}.run();
    return 0;
}
