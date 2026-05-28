# libcxxdes Documentation Index

[README](../README.md)

This index maps documentation topics to the source files and examples that define or demonstrate them.

Legend:

- `[md]` documentation
- `[ex]` example
- `[lib]` library source

## Start Here

| Topic | Use this for | Files |
| --- | --- | --- |
| Project overview | High-level motivation, feature list, and a first example. | `[md]` [README](../README.md) |
| Public include surface | The single include users are expected to depend on. | `[lib]` [cxxdes.hpp](../include/cxxdes/cxxdes.hpp) |
| Core API umbrella | The core coroutine, environment, awaitable, and composition definitions. | `[lib]` [core.hpp](../include/cxxdes/core/core.hpp) |
| Simulation wrapper | The CRTP helper behind `CXXDES_SIMULATION(...)`. | `[lib]` [simulation.hpp](../include/cxxdes/core/simulation.hpp) |

## Project And Development

| Topic | Use this for | Files |
| --- | --- | --- |
| Development notes | Open implementation and documentation tasks. | `[md]` [TODO](../TODO.md) |
| Test runner notes | Commands for running test configurations. | `[md]` [run_tests.md](../scripts/run_tests.md) |

## Core Simulation Model

| Topic | Use this for | Files |
| --- | --- | --- |
| Environment | Simulation time, event queue execution, `bind`, `run`, `run_for`, `run_until`, and deadline behavior. | `[lib]` [environment.ipp](../include/cxxdes/core/impl/environment.ipp) |
| Coroutine process | The main process abstraction, return values, priorities, latency, and awaiting. | `[lib]` [coroutine.ipp](../include/cxxdes/core/impl/coroutine.ipp) |
| Coroutine state | Internal state shared between coroutine handles, completion tokens, and parent links. | `[lib]` [coroutine_data.ipp](../include/cxxdes/core/impl/coroutine_data.ipp) |
| Coroutine model | Narrative overview of process scheduling, coroutine data, tokens, and the subroutine stack. | `[md]` [coroutine_model.md](coroutine_model.md) |
| Tokens | Scheduled resume points used by the environment event queue. | `[lib]` [token.ipp](../include/cxxdes/core/impl/token.ipp) |
| Basic usage | Binding several processes and running for a fixed duration. | `[ex]` [clocks.cpp](../examples/clocks.cpp) |
| Environment access | Getting the current environment from inside a coroutine. | `[ex]` [get_environment.cpp](../examples/get_environment.cpp) |
| Return values | Returning and awaiting values from `coroutine<T>`. | `[ex]` [return_value.cpp](../examples/return_value.cpp) |

## Time

| Topic | Use this for | Files |
| --- | --- | --- |
| Time expressions | Unit literals, precision conversion, and arithmetic such as `1_s + 500_ms`. | `[ex]` [time.cpp](../examples/time.cpp), `[lib]` [time.hpp](../include/cxxdes/misc/time.hpp) |
| Timeouts | Awaitables for relative delays, absolute time, lazy deadlines, and yielding. | `[lib]` [timeout.ipp](../include/cxxdes/core/impl/timeout.ipp) |
| Lazy deadlines | Capturing a deadline now and awaiting it later. | `[ex]` [lazy_timeout.cpp](../examples/lazy_timeout.cpp) |
| Real-valued model timing | Mapping random variables or real values into simulation time. | `[ex]` [producer_consumer.cpp](../examples/producer_consumer.cpp) |

## Awaitables

| Topic | Use this for | Files |
| --- | --- | --- |
| Awaitable contract | The functions a type must provide to participate in libcxxdes scheduling. | `[md]` [awaitables.md](awaitables.md), `[lib]` [awaitable.ipp](../include/cxxdes/core/impl/awaitable.ipp) |
| Await transformation | How coroutine promises bind awaitables to the current environment and priority. | `[lib]` [await_transform.ipp](../include/cxxdes/core/impl/await_transform.ipp) |
| Timeout awaitable | A minimal timeout-style awaitable and the production implementation. | `[md]` [awaitables.md](awaitables.md), `[lib]` [timeout.ipp](../include/cxxdes/core/impl/timeout.ipp) |
| Custom awaitable | A minimal custom awaitable that schedules a token. | `[ex]` [barebones_awaitable.cpp](../examples/barebones_awaitable.cpp) |
| Await transform extension | Adding custom `co_await` tags such as `get_env`. | `[ex]` [await_transform_extender.cpp](../examples/await_transform_extender.cpp) |

## Composition And Control Flow

| Topic | Use this for | Files |
| --- | --- | --- |
| Composition overview | Public helpers and operators: `all_of`, `any_of`, `sequential`, `async`, `&&`, `\|\|`, and `,`. | `[lib]` [compositions.ipp](../include/cxxdes/core/impl/compositions.ipp) |
| Composition token model | How `any_of` and `all_of` use child completion tokens and handlers. | `[md]` [coroutine_model.md](coroutine_model.md), `[lib]` [any_of.ipp](../include/cxxdes/core/impl/any_of.ipp) |
| Parallel composition | Waiting for the first or all awaitables to complete. | `[ex]` [any_of.cpp](../examples/any_of.cpp), `[lib]` [any_of.ipp](../include/cxxdes/core/impl/any_of.ipp) |
| Sequential composition | Awaiting a group of awaitables in order. | `[lib]` [sequential.ipp](../include/cxxdes/core/impl/sequential.ipp) |
| Async launch | Starting a coroutine and receiving it back without blocking the caller. | `[lib]` [async.ipp](../include/cxxdes/core/impl/async.ipp) |
| Dynamic composition | Range-based composition and larger control-flow expressions. | `[ex]` [complicated.cpp](../examples/complicated.cpp) |
| Reference/value behavior | Choosing by-value, by-reference, and rvalue-copy composition variants. | `[ex]` [compositions_lvalueref.cpp](../examples/compositions_lvalueref.cpp) |
| Capturing results | Assigning a coroutine result while composing it with other awaitables. | `[ex]` [capture_return.cpp](../examples/capture_return.cpp) |

## Subroutines

| Topic | Use this for | Files |
| --- | --- | --- |
| Subroutine implementation | Stack-like coroutine calls that run inside an existing process. | `[lib]` [subroutine.ipp](../include/cxxdes/core/impl/subroutine.ipp) |
| Subroutine stack model | How subroutines share the current process and use the coroutine data call stack. | `[md]` [coroutine_model.md](coroutine_model.md) |
| Usage patterns | Awaiting subroutines and converting them with `.as_coroutine()`. | `[ex]` [pitfall.cpp](../examples/pitfall.cpp) |
| Design experiment | Standalone coroutine stack experiment, useful for understanding the mechanism. | `[ex]` [subroutine.cpp](../examples/subroutine.cpp) |

## Synchronization Primitives

| Topic | Use this for | Files |
| --- | --- | --- |
| Synchronization overview | Primitive behavior and lifetime rules for blocked waiters. | `[md]` [sync_primitives.md](sync_primitives.md) |
| Event | Waiting until another process wakes all current waiters. | `[md]` [sync_primitives.md](sync_primitives.md#event), `[ex]` [event.cpp](../examples/event.cpp), `[lib]` [event.hpp](../include/cxxdes/sync/event.hpp) |
| Semaphore | Counting permits with `up()` and `down()`. | `[md]` [sync_primitives.md](sync_primitives.md#semaphore), `[ex]` [semaphore.cpp](../examples/semaphore.cpp), `[lib]` [semaphore.hpp](../include/cxxdes/sync/semaphore.hpp) |
| Queue | Blocking producer/consumer queues with optional bounded capacity. | `[md]` [sync_primitives.md](sync_primitives.md#queue), `[ex]` [queue.cpp](../examples/queue.cpp), `[lib]` [queue.hpp](../include/cxxdes/sync/queue.hpp) |
| Mutex | Exclusive access through an acquired handle that must be released. | `[md]` [sync_primitives.md](sync_primitives.md#mutex), `[ex]` [mutex.cpp](../examples/mutex.cpp), `[lib]` [mutex.hpp](../include/cxxdes/sync/mutex.hpp) |
| Resource | SimPy-style counted resource built on a semaphore. | `[md]` [sync_primitives.md](sync_primitives.md#resource), `[ex]` [resource.cpp](../examples/resource.cpp), `[lib]` [resource.hpp](../include/cxxdes/sync/resource.hpp) |

## Resource Acquisition Helpers

| Topic | Use this for | Files |
| --- | --- | --- |
| `_Co_with` helper | Concise acquire/body/release syntax for acquirable objects. | `[lib]` [co_with.ipp](../include/cxxdes/core/impl/co_with.ipp) |
| `_Coroutine` helper | Inline anonymous `coroutine<void>` creation. | `[lib]` [under_coroutine.ipp](../include/cxxdes/core/impl/under_coroutine.ipp) |
| Resource helper example | Using `_Co_with` with `resource`. | `[ex]` [raii_trick.cpp](../examples/raii_trick.cpp) |
| Shared-resource model | Using `_Co_with` to model shared memory bandwidth. | `[ex]` [basic_arch_sim.cpp](../examples/basic_arch_sim.cpp) |

## Debugging And Introspection

| Topic | Use this for | Files |
| --- | --- | --- |
| Debug macros | Macro-based debug/warning message hooks. | `[md]` [debug_macros.md](debug_macros.md), `[lib]` [helpers.hpp](../include/cxxdes/debug/helpers.hpp) |
| Debug scheduling | Small example showing timing and priority behavior. | `[ex]` [debug.cpp](../examples/debug.cpp) |
| Stack traces | Walking parent coroutine links and source locations. | `[ex]` [stack.cpp](../examples/stack.cpp) |

## Larger Examples

| Topic | Use this for | Files |
| --- | --- | --- |
| Producer-consumer queue | Queueing model with random arrivals/service times and measured latency. | `[ex]` [producer_consumer.cpp](../examples/producer_consumer.cpp) |
| ALOHA network simulation | Multiple stations, frame arrivals, collisions, and throughput. | `[ex]` [aloha.cpp](../examples/aloha.cpp) |
| Basic architecture simulation | Cache-like memory hierarchy with latency and shared bandwidth. | `[ex]` [basic_arch_sim.cpp](../examples/basic_arch_sim.cpp) |

## Pitfalls And Edge Cases

| Topic | Use this for | Files |
| --- | --- | --- |
| Object lifetime | Avoiding dangling object access when a coroutine outlives its owner. | `[ex]` [this_out_scope.cpp](../examples/this_out_scope.cpp) |
| Composition pitfalls | Reference lifetimes, subroutines, and composition edge cases. | `[ex]` [pitfall.cpp](../examples/pitfall.cpp) |
| Exception propagation | How exceptions move through awaited, composed, and async coroutines. | `[ex]` [exceptions.cpp](../examples/exceptions.cpp) |
| Stopping behavior | Running only part of a simulation with long-lived coroutines. | `[ex]` [interrupt_test.cpp](../examples/interrupt_test.cpp) |
