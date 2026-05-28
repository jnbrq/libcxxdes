# Synchronization Primitives

[README](../README.md) | [Documentation index: Synchronization Primitives](index.md#synchronization-primitives)

libcxxdes provides synchronization primitives for coordinating simulation processes.
They are awaitable building blocks layered on top of the core coroutine and token scheduler.

| Primitive | Purpose | Source | Example |
| --- | --- | --- | --- |
| `event` | Wait until another process wakes all current waiters. | [event.hpp](../include/cxxdes/sync/event.hpp) | [event.cpp](../examples/event.cpp) |
| `semaphore` | Count permits with `up()` and `down()`. | [semaphore.hpp](../include/cxxdes/sync/semaphore.hpp) | [semaphore.cpp](../examples/semaphore.cpp) |
| `queue<T>` | Block producers and consumers around an optional bounded capacity. | [queue.hpp](../include/cxxdes/sync/queue.hpp) | [queue.cpp](../examples/queue.cpp) |
| `mutex` | Provide exclusive access through an acquired handle. | [mutex.hpp](../include/cxxdes/sync/mutex.hpp) | [mutex.cpp](../examples/mutex.cpp) |
| `resource` | Model a counted SimPy-style resource. | [resource.hpp](../include/cxxdes/sync/resource.hpp) | [resource.cpp](../examples/resource.cpp) |

## Lifetime Rule

Synchronization primitives are ordinary C++ objects, but blocked coroutines may depend on them while suspended.
An `event::wait()` call creates a waiter token owned by the `event` until `event::wake()` schedules it into the environment.
Therefore, an `event` must outlive all coroutines currently waiting on it, unless those waiters have already been woken or the whole environment is being torn down.

The same rule applies indirectly to primitives built on `event`, including `semaphore`, `queue<T>`, `mutex`, and `resource`.
Do not destroy a synchronization primitive while live coroutines are still blocked on it.

Prefer storing synchronization primitives in the simulation object, in another owner whose lifetime covers the participating processes, or in a shared model object that outlives the waiters.

## Event

`event` is a one-shot wake-up mechanism for the processes currently waiting on it.
`wait()` blocks the current process.
`wake()` schedules all current waiters and clears the event's waiter list.
Processes that call `wait()` after a wake are not affected by earlier wake calls.

## Semaphore

`semaphore` stores a permit count and a maximum count.
`down()` waits until a permit is available and then decrements the count.
`up()` waits until the count can be increased and then increments it.

## Queue

`queue<T>` is a coroutine-friendly producer/consumer queue.
`put(...)` waits while a bounded queue is full, then constructs an item.
`pop()` waits while the queue is empty, then removes and returns the front item.

## Mutex

`mutex::acquire()` waits until the mutex is free and returns a move-only handle.
The handle must be released with `co_await handle.release()`.

## Resource

`resource` is a counted resource implemented on top of a semaphore.
`acquire()` returns a move-only handle, and the handle must be released with `co_await resource_handle.release()`.

## `_Co_with` Macro Syntax

The `_Co_with(resource) { ... }` helper is a macro around the lower-level acquire/body/release helper in [co_with.ipp](../include/cxxdes/core/impl/co_with.ipp).
Its expansion intentionally starts with `co_yield`:

```cpp
#define _Co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::subroutine<void>
```

The reason is C++ operator precedence.
`co_await x + body` would parse as `(co_await x) + body`, because `co_await` behaves like a unary operator.
`co_yield x + body` is a yield expression whose operand is the whole `x + body` expression, so the overloaded `operator+` can first build the acquire/body/release coroutine and then pass it through the coroutine promise's `yield_value`.

In normal user code this is just an implementation detail of `_Co_with`; write:

```cpp
_Co_with(resource) {
    co_await delay(5);
};
```

The trailing semicolon is still required because the macro expands to a coroutine statement.
