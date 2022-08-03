#define CXXDES_CO_WITH
#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;

CXXDES_SIMULATION(raii_trick) {
    sync::resource res{1};

    process<> q() {
        fmt::print("{} {}:{}", __PRETTY_FUNCTION__, __FILE__, __LINE__);
        co_return ;
    }

    process<> p(int id, time_type wait_time) {
        /*
        auto handle = co_await res.acquire();
        co_await timeout(wait_time);
        fmt::print("p id = {}, now = {}\n", id, now());
        co_await handle.release();
        */

        co_with(res) {
            co_await timeout(wait_time);
            fmt::print("p id = {}, now = {}\n", id, now());
        };
    }

    process<> co_main() {
        co_await (p(0, 10) && p(1, 5).priority(priority_consts::highest));
    }
};

int main() {
    raii_trick{}.run();
    return 0;
}
