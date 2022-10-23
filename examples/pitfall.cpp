#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(pitfall) {
    process<> p;

    process<> foo() {
        co_await delay(100);
    }

    process<> bar() {
        co_await p;
    }

    process<> co_main() {
        p = foo();
        co_await any_of.by_reference(p, bar());
        co_return ;
    }
};

#define _TrackLocation cxxdes::util::source_location const = cxxdes::util::source_location::current()

CXXDES_SIMULATION(interrupts) {
    process<> foo(_TrackLocation) {
        try {
            while (true) {
                fmt::print("now: {}\n", now());
                co_await delay(1);
            }
        }
        catch (interrupted_exception &err) {
            fmt::print("exception: {}\n", err.what());
            co_return ;
        }
    }

    process<> bar(_TrackLocation) {
        try {
            co_await delay(100);
        }
        catch (interrupted_exception &err) {
            fmt::print("exception: {}\n", err.what());
            co_return ;
        }
    }

    process<> co_main(_TrackLocation) {
        {
            process<> h = co_await async(foo());
            co_await delay(5);
            h.interrupt();
        }

        {
            process<> h = co_await async(bar());
            co_await delay(5);
            h.interrupt();
        }
    }
};

int main() {
    interrupts{}.run_for(500);
    return 0;
}
