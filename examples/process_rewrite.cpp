#include <fmt/core.h>

#include <cxxdes/core/process.hpp>
#include <cxxdes/core/compositions.hpp>
#include <cxxdes/core/timeout.hpp>

using namespace cxxdes::core;

process<int> g(int k = 10) {
    if (k == 0)
        co_return 0;
    co_return k + co_await g(k - 1);
}

process<int> f() {
    co_return ((co_await g()) + 10);
}

process<void> test() {
    auto this_env = co_await this_process::get_environment();
    co_await timeout(10);
    fmt::print("from {}, now = {}\n", __PRETTY_FUNCTION__, this_env->now());
    auto result = co_await f();
    fmt::print("from {}, now = {} and result = {}\n", __PRETTY_FUNCTION__, this_env->now(), result);
    auto priority = co_await this_process::get_priority();
    fmt::print("from {}, now = {} and priority = {}\n", __PRETTY_FUNCTION__, this_env->now(), priority);
}

process<void> test2() {
    auto this_env = co_await this_process::get_environment();
    co_await sequential(all_of(timeout(10), timeout(30)), timeout(5));
    fmt::print("from {}, now = {}\n", __PRETTY_FUNCTION__, this_env->now());
}

int main() {
    environment env;

    auto p = test2().priority(200);
    p.await_bind(&env);

    while (env.step()) ;

    return 0;
}
