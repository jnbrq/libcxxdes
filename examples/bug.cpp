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

int main() {
    des::environment env;
    env.bind(coro2());
    env.run();
    return 0;
}
