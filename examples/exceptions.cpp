#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

coroutine<> longfn()
{
    co_await timeout(100);
    co_return;
}

subroutine<> f()
{
    throw std::runtime_error("f has thrown an exception!");
    co_return;
}

subroutine<> g()
{
    co_await f();
}

coroutine<> h()
{
    try
    {
        co_await g();
    }
    catch (std::exception &ex)
    {
        fmt::println("[h] Exception: {}", ex.what());
    }

    throw std::runtime_error("h has thrown an exception!");
}

coroutine<> co_main1()
{
    try
    {
        co_await h();
    }
    catch (std::exception &ex)
    {
        fmt::println("[co_main1] Exception: {}", ex.what());
    }

    throw std::runtime_error("[co_main1] co_main has thrown an exception!");
}

coroutine<> co_main2()
{
    co_await async(longfn());

    try
    {
        co_await h();
    }
    catch (std::exception &ex)
    {
        fmt::println("[co_main2] Exception: {}", ex.what());
    }

    throw std::runtime_error("[co_main2] co_main has thrown an exception!");
}

void nestedExceptions1()
{
    fmt::println("test: nested exceptions 1");

    environment env;
    env.bind(co_main1());

    try
    {
        env.run();
    }
    catch (std::exception &ex)
    {
        fmt::println("[nestedExceptions1] Exception: {}", ex.what());
    }
}

void nestedExceptions2()
{
    fmt::println("test: nested exceptions 2");

    environment env;
    env.bind(co_main2());

    try
    {
        env.run();
    }
    catch (std::exception &ex)
    {
        fmt::println("[nestedExceptions2] Exception: {}", ex.what());
    }
}

void stopping()
{
    fmt::println("test: stopping");

    auto coro = []() -> coroutine<>
    {
        fmt::println("hey!");
        co_await timeout(50);
        // executes no more from here
        co_await timeout(100);
        fmt::println("should not be printed");
    }();

    environment env;
    env.bind(coro);

    env.run_for(1);
}

void asynchronous1()
{
    fmt::println("test: asynchronous1");

    auto coro = []() -> coroutine<>
    {
        auto coro2 = []() -> coroutine<>
        {
            co_await timeout(2);
            fmt::println("coro2 $0");
            throw std::runtime_error("coro2 has thrown an exception");
            fmt::println("coro2 $1");
        }();

        co_await (timeout(1) || coro2);
        fmt::println("coro done");
    }();

    environment env;
    env.bind(coro);

    try
    {
        env.run();
    }
    catch (std::exception &ex)
    {
        fmt::println("[asynchronous] Exception: {}", ex.what());
    }
}

void asynchronous2()
{
    fmt::println("test: asynchronous2");

    auto coro = []() -> coroutine<>
    {
        auto coro2 = []() -> coroutine<>
        {
            co_await timeout(2);
            fmt::println("coro2 $0");
            throw std::runtime_error("coro2 has thrown an exception");
            fmt::println("coro2 $1");
        }();
        co_await async(coro2);
        fmt::println("coro done");

        // since we did not await coro2, no completion token could
        // be generated. that is, the exception is lost.
    }();

    environment env;
    env.bind(coro);

    try
    {
        env.run();
    }
    catch (std::exception &ex)
    {
        fmt::println("[asynchronous2] Exception: {}", ex.what());
    }
}

int main()
{
    nestedExceptions1();
    nestedExceptions2();
    stopping();
    asynchronous1();
    asynchronous2();
    return 0;
}
