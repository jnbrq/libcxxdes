#include <gtest/gtest.h>
#include <vector>

// #define CXXDES_DEBUG_CORE_PROCESS
#define CXXDES_NO_DEPRECATED
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

TEST(ProcessTest, BasicFunctionality) {
    CXXDES_SIMULATION(test) {
        process<int> co_main() {
            co_return 10;
        }
    };

    EXPECT_EQ(test{}.run(), 10);
}

TEST(ProcessTest, OutOfScope) {
    CXXDES_SIMULATION(test) {
        process<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        process<int> co_main() {
            {
                auto p = foo();
                co_await any_of(p, delay(1));
                EXPECT_EQ(now(), 1);
                // p is now destroyed
                // foo() still runs
            }
            co_return 10;
            // co_main() is now done
        }
    };

    EXPECT_EQ(test{}.run(), 10);
}

TEST(ProcessTest, ReturnValueInspection) {
    CXXDES_SIMULATION(test) {
        process<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        process<int> co_main() {
            auto p = foo();
            co_await any_of(p, delay(1));
            EXPECT_FALSE(p.is_complete());
            EXPECT_EQ(now(), 1);

            co_await p;
            EXPECT_TRUE(p.is_complete());
            EXPECT_EQ(now(), 10);

            co_await p;
            EXPECT_TRUE(p.is_complete());
            EXPECT_EQ(now(), 10);
            
            co_return p.return_value();
        }
    };

    EXPECT_EQ(test{}.run(), 10);
}

TEST(ProcessTest, Latencies) {
    CXXDES_SIMULATION(test) {
        const time_type start_latency = 6;
        const time_type process_time = 5;
        const time_type return_latency = 8;

        process<int> f() {
            EXPECT_EQ(now(), start_latency);
            co_await delay(process_time);
            EXPECT_EQ(now(), start_latency + process_time);
            co_return 5;
        }

        process<int> co_main() {
            co_await f().
                latency(start_latency).
                return_latency(return_latency);
            EXPECT_EQ(now(), start_latency + process_time + return_latency);
            co_return 10;
        }
    };

    EXPECT_EQ(test{}.run(), 10);
}

TEST(ProcessTest, Priorities) {
    CXXDES_SIMULATION(test) {
        int counter = 100;

        process<void> foo(int t, int expected) {
            co_await delay(t);

            EXPECT_EQ(now(), t);
            EXPECT_EQ(counter, expected);
            counter = counter + 1;
        }

        process<void> spawn_foos(int expected) {
            co_await async(foo(1, expected));
            co_await async(foo(2, expected));
            co_await async(foo(3, expected));
        }

        process<void> reset_counter() {
            for (int i = 0; i < 4; ++i) {
                counter = 100;
                co_await delay(1);
            }
        }

        process<void> co_main() {
            co_await all_of(
                spawn_foos(100).priority(100),
                spawn_foos(101).priority(101),
                spawn_foos(102).priority(102),
                reset_counter().priority(0)
            );
        }
    };

    test{}.run();
}

TEST(ProcessTest, Recursion) {
    CXXDES_SIMULATION(test) {
        process<int> factorial(int k) {
            if (k == 0)
                co_return 1;
            co_return k * (co_await factorial(k - 1));
        }

        process<int> co_main() {
            co_return co_await factorial(6);
        }
    };

    EXPECT_EQ(test{}.run(), 720);
}

#ifdef CXXDES_SANITIZE_ADDRESS

TEST(ProcessTest, DanglingReference1) {
    CXXDES_SIMULATION(test) {
        process<void> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // when the async process starts to execute, x is already
            // destroyed; therefore, we have a dangling reference.
            co_await async.rvalue_by_value(x);
            
            co_return ;
        }
    };

    EXPECT_DEATH(test{}.run(), "");
}

#endif

TEST(ProcessTest, DanglingReference1Solution) {
    CXXDES_SIMULATION(test) {
        process<void> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // std::move(x) makes sure that x is moved into async.
            co_await async.rvalue_by_value(std::move(x));
            
            co_return ;
        }
    };

    test{}.run();
}

TEST(ProcessTest, NotDanglingReference1) {
    CXXDES_SIMULATION(test) {
        process<void> foo() {
            co_await delay(100);
        }

        process<void> co_main() {
            // this is created in the stack
            auto x = foo();

            // when the async process starts to execute, x is already
            // destroyed.
            // however, for processes, we have copy semantics and 
            co_await async(x);
            
            co_return ;
        }
    };

    test{}.run();
}

TEST(ProcessTest, ReturnProcess) {
    CXXDES_SIMULATION(test) {
        static process<void> g() {
            co_await delay(5);
            co_return ;
        }

        auto f(int && /* t */) {
            return g();
        }

        process<void> co_main() {
            auto p = f(1);
            co_await p;
            EXPECT_EQ(now(), 5);
            EXPECT_TRUE(p.is_complete());
        }
    };

    test{}.run();
}
