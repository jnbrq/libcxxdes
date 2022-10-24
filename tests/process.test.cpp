#include <gtest/gtest.h>
#include <vector>

#ifndef CXXDES_INTERRUPTABLE
#   define CXXDES_INTERRUPTABLE
#endif
// #define CXXDES_DEBUG_CORE_coroutine
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

TEST(ProcessTest, BasicFunctionality) {
    CXXDES_SIMULATION(test) {
        coroutine<int> foo() {
            co_return 10;
        }

        coroutine<> co_main() {
            auto r = co_await foo();
            EXPECT_EQ(r, 10);
        }
    };

    test::run();
}

TEST(ProcessTest, OutOfScope) {
    CXXDES_SIMULATION(test) {
        coroutine<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        coroutine<> co_main() {
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

    test::run();
}

#if 0
TEST(ProcessTest, ReturnValueInspection) {
    CXXDES_SIMULATION(test) {
        coroutine<int> foo() {
            co_await delay(10);
            co_return 10;
        }

        coroutine<> co_main() {
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

    test::run();
}
#endif

TEST(ProcessTest, Latencies) {
    CXXDES_SIMULATION(test) {
        const time_integral start_latency = 6;
        const time_integral coroutine_time = 5;
        const time_integral return_latency = 8;

        unique_coroutine<int> f() {
            EXPECT_EQ(now(), start_latency);
            co_await delay(coroutine_time);
            EXPECT_EQ(now(), start_latency + coroutine_time);
            co_return 5;
        }

        coroutine<> co_main() {
            co_await f().
                latency(start_latency).
                return_latency(return_latency);
            EXPECT_EQ(now(), start_latency + coroutine_time + return_latency);
        }
    };

    test::run();
}

TEST(ProcessTest, Priorities) {
    CXXDES_SIMULATION(test) {
        int counter = 100;

        coroutine<void> foo(int t, int expected) {
            co_await delay(t);

            EXPECT_EQ(now(), t);
            EXPECT_EQ(counter, expected);
            counter = counter + 1;
        }

        coroutine<void> spawn_foos(int expected) {
            co_await async(foo(1, expected));
            co_await async(foo(2, expected));
            co_await async(foo(3, expected));
        }

        coroutine<void> reset_counter() {
            for (int i = 0; i < 4; ++i) {
                counter = 100;
                co_await delay(1);
            }
        }

        coroutine<void> co_main() {
            co_await all_of(
                spawn_foos(100).priority(100),
                spawn_foos(101).priority(101),
                spawn_foos(102).priority(102),
                reset_counter().priority(0)
            );
        }
    };

    test::run();
}

TEST(ProcessTest, Recursion) {
    CXXDES_SIMULATION(test) {
        coroutine<int> factorial(int k) {
            if (k == 0)
                co_return 1;
            co_return k * (co_await factorial(k - 1));
        }

        coroutine<int> foo() {
            co_return co_await factorial(6);
        }
        
        coroutine<> co_main() {
            auto r = co_await foo();
            EXPECT_EQ(r, 720);
        }
    };

    test::run();
}

#ifdef CXXDES_SANITIZE_ADDRESS

TEST(ProcessTest, DanglingReference1) {
    CXXDES_SIMULATION(test) {
        coroutine<> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // when the async coroutine starts to execute, x is already
            // destroyed; therefore, we have a dangling reference.
            co_await async.copy_rvalue(x);
            
            co_return ;
        }
    };

    EXPECT_DEATH(test::run(), "");
}

#endif

TEST(ProcessTest, DanglingReference1Solution) {
    CXXDES_SIMULATION(test) {
        coroutine<> co_main() {
            // this is created in the stack
            auto x = delay(5);

            // std::move(x) makes sure that x is moved into async.
            co_await async.copy_rvalue(std::move(x));
            
            co_return ;
        }
    };

    test::run();
}

TEST(ProcessTest, NotDanglingReference1) {
    CXXDES_SIMULATION(test) {
        coroutine<> foo() {
            co_await delay(100);
        }

        coroutine<> co_main() {
            // this is created in the stack
            auto x = foo();

            // when the async coroutine starts to execute, x is already
            // destroyed.
            // however, for coroutinees, we have copy semantics and 
            co_await async(x);
            
            co_return ;
        }
    };

    test::run();
}

TEST(ProcessTest, Returncoroutine) {
    CXXDES_SIMULATION(test) {
        static coroutine<int> g() {
            co_await delay(5);
            co_return 5;
        }

        auto f(int && /* t */) {
            return g();
        }

        coroutine<> co_main() {
            auto p = f(1);
            co_await p;
            EXPECT_EQ(now(), 5);
            EXPECT_TRUE(p.complete());
            EXPECT_EQ(p.return_value(), 5);
        }
    };

    test::run();
}

TEST(ProcessTest, Interrupt) {
    CXXDES_SIMULATION(test) {
        test(environment &env): simulation(env) {
        }

        bool flag = false;

        coroutine<> foo() {
            while (true) {
                co_await delay(10);
            }
        }

        coroutine<> bar() {
            try {
                while (true)
                    co_await delay(1000);
            }
            catch (stopped_exception & /* ex */) {
                flag = true;
                co_return ;
            }
        }

        coroutine<> co_main() {
            co_await async(foo());
            co_await async(bar());
            
            while (true) {
                co_await delay(10);
            }
        }
    };

    environment env;
    test t{env};
    env.run_for(100);
    env.reset();
    EXPECT_TRUE(t.flag);
}
