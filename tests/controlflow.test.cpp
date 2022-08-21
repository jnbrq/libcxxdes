#include <gtest/gtest.h>

#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

TEST(ControlFlowTest, Async1) {
    CXXDES_SIMULATION(test) {
        process<void> co_main() {
            auto x = co_await async(delay(100));
            EXPECT_EQ(now(), 0);

            co_await delay(5);
            EXPECT_EQ(now(), 5);

            co_await x;
            EXPECT_EQ(now(), 100);
        }
    };

    test{}.run();
}

TEST(ControlFlowTest, Async2) {
    CXXDES_SIMULATION(test) {
        cxxdes::sync::event evt;

        process<void> bar() {
            co_await delay(50);
            co_await evt.wake();
        }

        process<void> foo() {
            co_await async(bar());
            EXPECT_EQ(now(), 0);
        }

        process<void> co_main() {
            auto x = foo();
            auto y = co_await async(x);
            EXPECT_EQ(now(), 0);

            co_await delay(5);
            EXPECT_EQ(now(), 5);

            co_await y;
            EXPECT_EQ(now(), 5);

            co_await evt.wait();
            EXPECT_EQ(now(), 50);
        }
    };

    test{}.run();
}
