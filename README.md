# libcxxdes

libcxxdes is a C++20 Discrete Event Simulation library inspired by [SimPy](https://simpy.readthedocs.io/en/latest/). Just like Simpy, libcxxdes models discrete-event systems using processes and control-flow expressions. While its API feels familiar to SimPy users, being written in C++, it offers up to a few orders of magnitude performance improvement. Furthermore, discrete-event systems modeled using libcxxdes can be later integrated into other DES simulators whose kernels are written in C or C++, such as [SystemC](https://systemc.org/), [gem5](https://www.gem5.org/) or [OMNET++](https://omnetpp.org/). 

What sets libcxxdes apart from these simulators is its use of modern C++ features to facilitate system modeling, most prominently the newly introduced [coroutine](https://en.cppreference.com/w/cpp/language/coroutines) support.

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
            co_await timeout(lambda());
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
            co_await timeout(mu());

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

libcxxdes tries to provide the complete feature set of SimPy, it currently supports/provides:

1. Processes modeled using `coroutine<T>`s.
2. Control flow expressions:
    - Parallel compositions:
    `co_await all_of(p1(), p2(), ...)`, `co_await (p1() && p2)`,
    `co_await any_of(p1(), p2(), ...)`, `co_await (p1() || p2())`
    - Sequential compositions:
    `co_await sequential(p1(), p2(), ...)`, `co_await (p1(), p2(), ...)`
    - Timeouts:
    `co_await delay(5)`, `co_await timeout(5_s)`
3. Interruptable coroutines which are useful for modelling preemptive resources.
4. Priority-scheduling of events that take place at the same simulation time. `coroutine<T>` can be assigned priorities! (lower the number, higher the priority)
5. `time_unit()` and `time_precision()` functions for mapping simulation time (integer) to real-world time.
6. Synchronization primitives, such as `mutex`, `semaphore`, `queue<T>`s, and `event`.
7. RAII-style acquisition of resources using `co_with (resource) { /*  */ }` syntax.
8. SimPy-compatible `resource`, `container` and `preemptive_resource`.
9. Debugging facilities, such as getting the stack traces of `coroutine<T>`s.
10. A template-metaprogramming-based DSL to describe time accurately without suffering from the quirks of the floating-point aritmetic. `1_s + 500_ms + 100_us` is mapped to `1'500'100` simulation if the precision is set to `1_us` or to `1'500` for a precision of `1_ms`.
11. A CMake-based build system to facilitate integrating withn other projects.
