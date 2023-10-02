#define CXXDES_DEBUG_TOKEN
#include <cxxdes/cxxdes.hpp>

namespace des {
    using namespace cxxdes::core;
}

des::coroutine<> coro1() {
    co_await des::timeout(1);
}

des::coroutine<> coro2() {
    co_await (coro1() || coro1());
}

#if 0

int main() {
    des::environment env;
    env.bind(coro2());
    env.run();
    return 0;
}

#endif

#if 1

des::coroutine<> clocks(int i) {
    co_await des::timeout(i);
    while (true) {
        co_await des::timeout(5);
    }
}

des::coroutine<> co_main() {
    co_await des::async(clocks(1));
    co_await clocks(0);
}

int main() {
    des::environment env;
    
    env.bind(clocks(0));
    env.bind(clocks(1));
    
    // env.bind(co_main());
    env.run_for(50);
    return 0;
}

#endif
