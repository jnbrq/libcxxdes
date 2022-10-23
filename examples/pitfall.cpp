#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(pitfall) {
    coroutine<> p;

    coroutine<> foo() {
        co_await delay(100);
    }

    coroutine<> bar() {
        co_await p;
    }

    coroutine<> co_main() {
        p = foo();
        co_await any_of.by_reference(p, bar());
        co_return ;
    }
};

#define _TrackLocation cxxdes::util::source_location const = cxxdes::util::source_location::current()

CXXDES_SIMULATION(interrupts) {
    coroutine<> foo(_TrackLocation) {
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

    coroutine<> bar(_TrackLocation) {
        try {
            co_await delay(100);
        }
        catch (interrupted_exception &err) {
            fmt::print("exception: {}\n", err.what());
            co_return ;
        }
    }

    coroutine<> co_main(_TrackLocation) {
        {
            coroutine<> h = co_await async(foo());
            co_await delay(5);
            h.interrupt();
        }

        {
            coroutine<> h = co_await async(bar());
            co_await delay(5);
            h.interrupt(interrupted_exception("interrupted!"));
            // h.interrupt(std::runtime_error("what?"));
        }
    }
};

CXXDES_SIMULATION(subroutines) {
    subroutine<int> bar() {
        fmt::print("bar\n");
        co_return 10;
    }
    
    subroutine<int> foo() {
        fmt::print("fooA\n");
        fmt::print("i = {}\n", co_await bar());
        fmt::print("fooB\n");
        co_await delay(10);
        co_return 20;
    }

    coroutine<> co_main() {
        fmt::print("co_mainA now {}\n", now());
        fmt::print("j = {}\n", co_await foo());
        fmt::print("co_mainB now = {}\n", now());
        co_return ;
    }
};

int main() {
    fmt::print("pitfall\n");
    pitfall().run();

    fmt::print("interrupts\n");
    interrupts().run_for(500);

    fmt::print("subroutines\n");
    subroutines().run();
    return 0;
}