#include <fmt/core.h>
#include <cxxdes/time.hpp>
#include <cxxdes/core/timeout.hpp>

using namespace std;
using namespace cxxdes::time_ops;

int main() {
    constexpr auto a = 1_s;
    constexpr auto b = 1_ms;
    constexpr auto c = 1_ns;

    constexpr auto x = (-a + 3*b).count(c);
    constexpr auto y = (1_s + (7 * b - 6_s)).count(b);

    auto test1 = cxxdes::core::timeout(a);

    fmt::print("{} {}\n", x, y);

    return 0;
}
