#include <cxxdes/cxxdes.hpp>

#include <fmt/core.h>

using namespace cxxdes;
using namespace cxxdes::core;

unique_process<> f() {
    co_return ;
}

unique_process<int> test() {
    co_await f();
    co_return 10;
}

#include <assert.h>

int main(int argc, char **argv) {
    auto p = test();
    auto p2 = std::move(p);
    assert(not (bool) p);
    assert((bool) p2);
}