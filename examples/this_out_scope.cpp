#include <cxxdes/cxxdes.hpp>
#include <fmt/format.h>

using namespace cxxdes::core;

struct test {
    coroutine<> p() {
        co_await delay(10);
        x_ = 8; // x_ is already destroyed at this point
        co_return ;
    }

private:
    volatile int x_ = 10;
};

CXXDES_SIMULATION(this_out_scope) {
    coroutine<> co_main() {
        {
            test t;
            co_await (t.p() || delay(1));
        }

        co_await yield();
    }
};

int main() {
    this_out_scope{}.run();
    return 0;
}
