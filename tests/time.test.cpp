#include <gtest/gtest.h>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::time_utils::ops;

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

TEST(TimeTest, TimeExpr) {
    using time_expr = cxxdes::time_utils::time_expr<>;

    constexpr auto prec = 1_ms;
    time_expr a = 1_s;
    EXPECT_EQ(a.count(prec), 1000);
    EXPECT_EQ((a + 5_ms).count(prec), 1005);
    EXPECT_EQ(a.count(prec), 1000);
    EXPECT_EQ((a + 2_ms).count(prec), 1002);
    EXPECT_EQ((a + a).count(prec), 2000);

    time_expr b = 2 * a + a / 10;
    EXPECT_EQ(b.count(prec), 2100);

    time_expr c = 5;
    EXPECT_EQ(c.count(prec, prec), 5);
}
