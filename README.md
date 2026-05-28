# libcxxdes

[Documentation index](docs/index.md)

libcxxdes is a C++20 discrete event simulation library inspired by [SimPy](https://simpy.readthedocs.io/en/latest/).
Like SimPy, libcxxdes models discrete-event systems using processes and control-flow expressions.
Because it is written in C++, libcxxdes can integrate with other C and C++ simulation kernels such as [SystemC](https://systemc.org/), [gem5](https://www.gem5.org/), or [OMNET++](https://omnetpp.org/).

The library uses C++20 [coroutines](https://en.cppreference.com/w/cpp/language/coroutines) to express simulation processes directly in C++ control flow.

For a navigable map of the current documentation and examples, see [docs/index.md](docs/index.md).

## A Quick Example (`producer_consumer.cpp`)

The following example demonstrates the modeling of an M/M/1 queueing system:

```cpp

using namespace cxxdes::core;
using namespace cxxdes::core::time_ops;

CXXDES_SIMULATION(producer_consumer_example) {
    /* ... */
    
    coroutine<> producer() {
        for (std::size_t i = 0; i < n_packets; ++i) {
            // places an item to the queue
            co_await q.put(now_seconds());

            // models the arrival time (exponential random variable)
            
            // env.timeout ensures that lambda() is in time units of the environment.
            // It is equivalent to lambda() * 1_s in this case.
            co_await env.timeout(lambda());
        }
    }

    coroutine<> consumer() {
        std::size_t n = 0;

        while (true) {
            // blocks until an item is in the queue
            auto x = co_await q.pop();
            ++n;

            if (n == n_packets) {
                // end of the experiment
                // calculate the average latency
                avg_latency = total_latency / n_packets;
                co_return ;
            }
            
            // models the service time (exponential random variable)
            co_await env.timeout(mu());

            total_latency += (now_seconds() - x);
        }
    }

    coroutine<> co_main() {
        // start both producer() and consumer() in parallel
        co_await (producer() && consumer());
    }
};
```

## Features

libcxxdes currently supports:

1. Processes modeled using `coroutine<T>`s.
2. Control flow expressions:
    - Parallel compositions:
    `co_await all_of(p1(), p2(), ...)`, `co_await (p1() && p2)`,
    `co_await any_of(p1(), p2(), ...)`, `co_await (p1() || p2())`
    - Sequential compositions:
    `co_await sequential(p1(), p2(), ...)`, `co_await (p1(), p2(), ...)`
    - Timeouts:
    `co_await delay(5)`, `co_await timeout(5_s)`
3. Priority scheduling for events that take place at the same simulation time.
4. `time_unit()` and `time_precision()` functions for mapping integer simulation time to real-world units.
5. Synchronization primitives, including `event`, `semaphore`, `queue<T>`, `mutex`, and `resource`.
6. Resource acquisition helpers using `_Co_with(resource) { ... }`.
7. Debugging and introspection facilities, including coroutine stack traces.
8. A template-metaprogramming-based time DSL for expressions such as `1_s + 500_ms + 100_us`.
9. A CMake interface target for integrating the library into other projects.
