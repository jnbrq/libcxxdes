#include <cxxdes/cxxdes.hpp>

#include <fmt/core.h>

using namespace cxxdes;
using namespace cxxdes::core;

unique_coroutine<> f() {
    co_return ;
}

unique_coroutine<int> test() {
    co_await f();
    co_return 10;
}

#include <assert.h>

int main(int, char **) {
    auto p = test();
    auto p2 = std::move(p);
    assert(not (bool) p);
    assert((bool) p2);
}
