#include <fmt/core.h>

#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

coroutine<int> g(int k = 10) {
    if (k == 0)
        co_return 0;
    co_return k + co_await g(k - 1);
}

coroutine<int> f() {
    co_return ((co_await g()) + 10);
}

coroutine<void> test() {
    auto this_env = co_await this_environment();
    co_await sequential(all_of(timeout(10), timeout(30)), timeout(5));
    fmt::print("from {}, now = {}\n", __PRETTY_FUNCTION__, this_env->now());
    auto result = co_await f();
    fmt::print("from {}, now = {} and result = {}\n", __PRETTY_FUNCTION__, this_env->now(), result);
    auto priority = (co_await this_coroutine())->priority();
    fmt::print("from {}, now = {} and priority = {}\n", __PRETTY_FUNCTION__, this_env->now(), priority);
}

int main() {
    environment env;

    auto p = test().priority(200);
    p.await_bind(&env);

    while (env.step()) ;

    return 0;
}
