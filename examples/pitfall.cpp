#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

CXXDES_SIMULATION(pitfall) {
    using simulation::simulation;

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

CXXDES_SIMULATION(subroutines) {
    using simulation::simulation;

    subroutine<> f() {
        fmt::print("f\n");
        co_return ;
    }

    subroutine<int> bar() {
        fmt::print("barA\n");
        co_await f();
        fmt::print("barB\n");
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

CXXDES_SIMULATION(subroutines2) {
    using simulation::simulation;

    subroutine<int> foo() {
        co_await delay(600);
        co_await delay(600);
        co_return 20;
    }

    coroutine<> co_main() {
        fmt::print("co_mainA now {}\n", now());
        fmt::print("j = {}\n", co_await foo());
        fmt::print("co_mainB now = {}\n", now());
        co_return ;
    }
};

CXXDES_SIMULATION(subroutines3) {
    using simulation::simulation;

    subroutine<> foo() {
        co_await delay(50);
        co_return ;
    }

    coroutine<> bar() {
        co_await delay(100);
        co_return ;
    }

    coroutine<> co_main() {
        // note: if subroutine is stored in a variable, use as std::move(s).as_coroutine()
        co_await (foo().as_coroutine() && bar());
        fmt::print("now = {}\n", now());
        co_return ;
    }
};

CXXDES_SIMULATION(subroutines4) {
    using simulation::simulation;

    subroutine<> bar(int i = 100) {
        if (i == 0)
            co_return ;
        co_await delay(100);
        co_await bar(i - 1);
        co_return ;
    }

    coroutine<> co_main() {
        // note: if subroutine is stored in a variable, use as std::move(s).as_coroutine()
        co_await bar(10'000'000);
        fmt::print("now = {}\n", now());
        co_return ;
    }
};

int main() {
    fmt::print("pitfall\n");
    pitfall{}.run();

    fmt::print("subroutines\n");
    subroutines{}.run();

    fmt::print("subroutines2\n");
    subroutines2{}.run_for(500);

    fmt::print("subroutines3\n");
    subroutines3{}.run_for(500);

    fmt::print("subroutines4\n");
    subroutines4{}.run();

    return 0;
}
