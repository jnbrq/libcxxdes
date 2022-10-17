#include <gtest/gtest.h>
#include <vector>

#ifndef CXXDES_INTERRUPTABLE
#   define CXXDES_INTERRUPTABLE
#endif
// #define CXXDES_DEBUG_CORE_PROCESS
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

TEST(ProcessTest, BasicFunctionality) {
    CXXDES_SIMULATION(test) {
        process<int> foo() {
            co_return 10;
        }

        process<> co_main() {
            auto r = co_await foo();
            EXPECT_EQ(r, 10);
        }
    };

    test{}.run();
}

TEST(ProcessTest, OutOfScope) {
    CXXDES_SIMULATION(test) {
        process<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        process<> co_main() {
            {
                auto p = foo();
                co_await any_of(p, delay(1));
                EXPECT_EQ(now(), 1);
                // p is now destroyed
                // foo() still runs
            }

            // co_main() is now done
        }
    };

    test{}.run();
}

#if 0
TEST(ProcessTest, ReturnValueInspection) {
    CXXDES_SIMULATION(test) {
        process<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        process<> co_main() {
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
            EXPECT_EQ(p.return_value(), 10);
        }
    };

    test{}.run();
}
#endif

TEST(ProcessTest, Latencies) {
    CXXDES_SIMULATION(test) {
        const time_integral start_latency = 6;
        const time_integral process_time = 5;
        const time_integral return_latency = 8;

        process<int> f() {
            EXPECT_EQ(now(), start_latency);
            co_await delay(process_time);
            EXPECT_EQ(now(), start_latency + process_time);
            co_return 5;
        }

        process<> co_main() {
            co_await f().
                latency(start_latency).
                return_latency(return_latency);
            EXPECT_EQ(now(), start_latency + process_time + return_latency);
        }
    };

    test{}.run();
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

        process<int> foo() {
            co_return co_await factorial(6);
        }
        
        process<> co_main() {
            auto r = co_await foo();
            EXPECT_EQ(r, 720);
        }
    };

    test{}.run();
}

#ifdef CXXDES_SANITIZE_ADDRESS

TEST(ProcessTest, DanglingReference1) {
    CXXDES_SIMULATION(test) {
        process<> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // when the async process starts to execute, x is already
            // destroyed; therefore, we have a dangling reference.
            co_await async.copy_rvalue(x);
            
            co_return ;
        }
    };

    EXPECT_DEATH(test{}.run(), "");
}

#endif

TEST(ProcessTest, DanglingReference1Solution) {
    CXXDES_SIMULATION(test) {
        process<> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // std::move(x) makes sure that x is moved into async.
            co_await async.copy_rvalue(std::move(x));
            
            co_return ;
        }
    };

    test{}.run();
}

TEST(ProcessTest, NotDanglingReference1) {
    CXXDES_SIMULATION(test) {
        process<> foo() {
            co_await delay(100);
        }

        process<> co_main() {
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

        process<> co_main() {
            auto p = f(1);
            co_await p;
            EXPECT_EQ(now(), 5);
            EXPECT_TRUE(p.is_complete());
        }
    };

    test{}.run();
}

TEST(ProcessTest, Interrupt) {
    CXXDES_SIMULATION(test) {
        bool flag = false;

        process<> foo() {
            while (true) {
                co_await delay(10);
            }
        }

        process<> bar() {
            try {
                while (true)
                    co_await delay(1000);
            }
            catch (interrupted_exception & /* ex */) {
                flag = true;
                co_return ;
            }
        }

        process<> co_main() {
            while (true) {
                co_await delay(10);
            }
            // co_await foo();
        }
    };

    auto obj = test{};
    obj.run_for(100).stop();
    // EXPECT_TRUE(obj.flag);
}
