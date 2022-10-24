#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(raii_trick) {
    cxxdes::sync::resource res{1};
    
    coroutine<> p(int id, time_integral wait_time) {
        /*
        auto handle = co_await res.acquire();
        co_await timeout(wait_time);
        fmt::print("p id = {}, now = {}\n", id, now());
        co_await handle.release();
        */

        fmt::print("p id = {}, priority = {}\n", id, (co_await this_coroutine())->priority());

        _Co_with(res) {
            co_await timeout(wait_time);
            fmt::print("p id = {}, now = {}\n", id, now());
        };
    }

    coroutine<> co_main() {
        co_await all_of(p(0, 10).priority(0), p(1, 5).priority(-1), p(2, 8).priority(-2), p(3, 8).priority(-3));
    }
};

int main() {
    raii_trick::run();
    return 0;
}
