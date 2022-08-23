#include <gtest/gtest.h>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::time_ops;

TEST(TimeTest, BasicFunctionality) {
    constexpr auto a = 1_s;
    constexpr auto b = 1_ms;
    constexpr auto c = 1_ns;

    constexpr auto x = a + b + c;
    EXPECT_EQ(x.count(b), 1001);

    constexpr auto y = -a + 3 * b;
    EXPECT_EQ(y.count(b), -997);
    EXPECT_EQ(y.count(c), -997'000'000);

    constexpr auto z = 1_s + (7 * b - 6_s);
    EXPECT_EQ(z.count(b), -4993);

    {
        constexpr auto w = x + x;
        EXPECT_EQ(w.count(b), 2002);
    }

    {
        constexpr auto w = x / 2;
        EXPECT_EQ(w.count(1_us), 500500);
    }

    // TODO add more tests
    // TODO add tests for checking the sizes in compile time
}
