#include <gtest/gtest.h>
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

TEST(ControlFlowTest, Async1) {
    CXXDES_SIMULATION(test) {
        process<> co_main() {
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

        process<> bar() {
            co_await delay(50);
            co_await evt.wake();
        }

        process<> foo() {
            co_await async(bar());
            EXPECT_EQ(now(), 0);
        }

        process<> co_main() {
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

TEST(ControlFlowTest, Compositions1) {
    CXXDES_SIMULATION(test) {
        process<> co_main() {
            time_integral expected_now = 0;

            co_await all_of(timeout(10), timeout(20));
            expected_now += 20;
            EXPECT_EQ(now(), expected_now);

            co_await any_of(timeout(10), timeout(20));
            expected_now += 10;
            EXPECT_EQ(now(), expected_now);

            co_await ((delay(1000) && delay(5)) || (delay(100) && delay(1)));
            expected_now += 100;
            EXPECT_EQ(now(), expected_now);

            co_await (delay(10), delay(20), delay(30));
            expected_now += 60;
            EXPECT_EQ(now(), expected_now);

            {
                auto a = (delay(1000) && delay(5)) || (delay(100) && delay(1));
                co_await a;
                expected_now += 100;
                EXPECT_EQ(now(), expected_now);
            }

            {
                auto a = (delay(10), delay(20), delay(30));
                co_await a;
                expected_now += 60;
                EXPECT_EQ(now(), expected_now);
            }
        }
    };

    test{}.run();
}

TEST(ControlFlowTest, Compositions2) {
    CXXDES_SIMULATION(test) {
        process<> co_main() {
            time_integral expected_now = 0;

            co_await all_of(async(delay(10)), async(delay(20)));
            expected_now += 0;
            EXPECT_EQ(now(), expected_now);

            auto x = co_await async(all_of(delay(10), delay(100)));
            expected_now += 0;
            EXPECT_EQ(now(), expected_now);

            co_await x;
            expected_now += 100;
            EXPECT_EQ(now(), expected_now);
        }
    };

    test{}.run();
}

TEST(ControlFlowTest, AwaitableFactory) {
    struct factory_test {
        process<void> await() {
            co_await delay(100);
        }
    };

    CXXDES_SIMULATION(test) {
        process<> co_main() {
            auto f = factory_test{};
            co_await f;
            EXPECT_EQ(now(), 100);
        }
    };

    test{}.run();
}
