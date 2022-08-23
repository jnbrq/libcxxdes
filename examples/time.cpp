#include <fmt/core.h>
#include <cxxdes/misc/time.hpp>
#include <cxxdes/core/timeout.hpp>

using namespace std;
using namespace cxxdes::time_ops;

int main() {
    constexpr auto a = 1_s;
    constexpr auto b = 1_ms;
    constexpr auto c = 1_ns;

    constexpr auto u = -a + 3*b;
    constexpr auto v = 1_s + (7 * b - 6_s);

    constexpr auto x = u.count(c);
    constexpr auto y = v.count(b);

    [[maybe_unused]] auto test1 = cxxdes::core::timeout(a);

    fmt::print("{} {}\n", x, y);

    fmt::print("sizeof(a) = {}\n", sizeof(a));
    fmt::print("sizeof(b) = {}\n", sizeof(b));
    fmt::print("sizeof(c) = {}\n", sizeof(c));
    fmt::print("sizeof(u) = {}\n", sizeof(u));
    fmt::print("sizeof(v) = {}\n", sizeof(v));
    fmt::print("sizeof(x) = {}\n", sizeof(x));
    fmt::print("sizeof(y) = {}\n", sizeof(y));

    return 0;
}
