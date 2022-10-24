#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes;
using namespace cxxdes::core;

CXXDES_SIMULATION(stack) {
    coroutine<> f1() {
        co_await f2();
    }

    coroutine<> f2() {
        co_await f3();
    }

    coroutine<> f3() {
        co_await f4();
    }

    coroutine<> f4() {
        co_await stacktrace();
    }

    coroutine<> stacktrace() {
        coroutine_data_ptr h = (co_await this_coroutine()).cast<coroutine_data>();
        unsigned i = 0;
        while (h) {
            auto loc = h->loc_awaited();
            if (loc) {
                fmt::print("[{}] {} @ {}:{}\n", i, h->loc_awaited().function_name, h->loc_awaited().file, h->loc_awaited().line);
            }
            else {
                fmt::print("[{}] <none>\n", i);
            }
            h = h->parent();
            ++i;
        }
        co_return ;
    }

    coroutine<> co_main() {
        co_await f1();
    }
};

int main() {
    stack::run();
}
