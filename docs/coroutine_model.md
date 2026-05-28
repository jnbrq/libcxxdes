# Coroutine Model

[README](../README.md) | [Documentation index: Core Simulation Model](index.md#core-simulation-model)

libcxxdes models simulation processes with C++20 coroutines.
The public type users usually write is `cxxdes::core::coroutine<T>`, while the scheduler works through shared coroutine state stored in `coroutine_data`.

This page explains the execution model behind processes, subroutines, tokens, and the coroutine call stack.

## Processes And Coroutine Data

A `coroutine<T>` is a schedulable simulation process.
It can be bound to an `environment`, awaited by another coroutine, assigned a priority or start latency, and optionally return a value.

| Concept          | Role                                                       | Source                                                               |
| ---------------- | ---------------------------------------------------------- | -------------------------------------------------------------------- |
| `coroutine<T>`   | User-facing process handle and awaitable process object.   | [coroutine.ipp](../include/cxxdes/core/impl/coroutine.ipp)           |
| `coroutine_data` | Shared process state resumed by the scheduler.             | [coroutine_data.ipp](../include/cxxdes/core/impl/coroutine_data.ipp) |
| `environment`    | Owns the event queue and resumes scheduled coroutine data. | [environment.ipp](../include/cxxdes/core/impl/environment.ipp)       |

When a coroutine function is called, it creates a coroutine object, but it does not immediately run.
Its promise creates a `coroutine_data` object and stores the initial coroutine handle in that data object.

That initial handle is the first entry in the `coroutine_data` call stack.
It is created in the `coroutine<T>::promise_type` constructor with `std::coroutine_handle<promise_type>::from_promise(*this)`, then pushed with `coroutine_data::push_coro_(...)`.
At this point there are no subroutines yet; the call stack contains only the process coroutine itself.

In code, the first call-stack entry is created here:

```cpp
auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
coro_data = new coroutine_data_type(loc);
coro_data->push_coro_(coro);
```

`push_coro_(...)` is the first place where `call_stack_` is emplaced.
It calls `call_stack_.push_back(coro)` and sets `should_continue_ = true`.
The same helper is later reused when a `subroutine<T>` is awaited, but for a newly created `coroutine<T>` this initial push comes from the process promise constructor itself.

See [`coroutine<T>::promise_type`](../include/cxxdes/core/impl/coroutine.ipp#L238) and [`coroutine_data::push_coro_`](../include/cxxdes/core/impl/coroutine_data.ipp#L108).

The important state in `coroutine_data` includes:

- the bound `environment`
- the coroutine call stack
- the inherited priority and start latency
- the parent coroutine, if this coroutine was started by another coroutine
- source locations used for introspection
- completion state and return storage

## Binding And Starting

A process starts when it is bound to an environment, either explicitly with `env.bind(p)` or implicitly when another coroutine awaits it.

Binding records the environment, captures the current parent coroutine, applies inherited priority if needed, and schedules an initial token at:

```cpp
env.now() + coroutine_data::latency()
```

That token is the first event that will resume the coroutine.
The binding logic is defined in [`coroutine_data::bind_`](../include/cxxdes/core/impl/environment.ipp#L204).

Examples:

- [clocks.cpp](../examples/clocks.cpp) binds several processes manually.
- [return_value.cpp](../examples/return_value.cpp) awaits one coroutine from another.
- [get_environment.cpp](../examples/get_environment.cpp) retrieves the current environment from inside a coroutine.

## Tokens

The environment does not directly run arbitrary code.
It repeatedly pops the next scheduled `token` from its priority queue and uses that token to decide what to resume or invoke.

| Token field | Meaning                                                                     |
| ----------- | --------------------------------------------------------------------------- |
| `time`      | Simulation time when the token becomes active.                              |
| `priority`  | Tie-breaker for tokens at the same simulation time; lower values run first. |
| `coro_data` | Coroutine state to resume.                                                  |
| `handler`   | Optional custom handler used by compositions such as `any_of` and `all_of`. |
| `eptr`      | Optional exception to rethrow during scheduling.                            |

See [token.ipp](../include/cxxdes/core/impl/token.ipp) and [environment.ipp](../include/cxxdes/core/impl/environment.ipp).

## Resuming A Coroutine

The environment resumes coroutines through `environment::step()`.
A single step does the following:

1. Return `false` if there are no scheduled tokens.
2. Pop the next token from the priority queue.
3. Advance `now_` to the token time if needed.
4. Invoke a token handler, resume coroutine data, or rethrow a stored exception.
5. Return `true`.

For normal coroutine execution, the token has a `coro_data` pointer.
The environment temporarily records it as `current_coroutine_`, calls `coro_data->resume()`, then clears `current_coroutine_`.

`coroutine_data::resume()` resumes the top coroutine handle in its explicit call stack:

```cpp
do {
    should_continue_ = false;
    auto top = call_stack_.back();
    top.resume();
} while (should_continue_);
```

The loop matters for subroutines.
Entering a subroutine pushes another coroutine handle and sets `should_continue_`; returning from a subroutine pops that handle and sets `should_continue_` again.
This lets a single environment resume step move through ordinary nested helper calls until execution reaches an awaitable that actually suspends the process.

The process coroutine itself is not popped from the call stack.
Its `final_suspend()` is `std::suspend_never`, and process completion is represented by `coroutine_data::do_return()`, which schedules completion tokens, marks the process complete, and removes it from the environment-managed coroutine set.

| Mechanism                      | Source                                                                              |
| ------------------------------ | ----------------------------------------------------------------------------------- |
| Environment resume step        | [`environment::step`](../include/cxxdes/core/impl/environment.ipp#L66)              |
| Coroutine data resume loop     | [`coroutine_data::resume`](../include/cxxdes/core/impl/coroutine_data.ipp#L20)      |
| Initial process frame creation | [`coroutine<T>::promise_type`](../include/cxxdes/core/impl/coroutine.ipp#L238)      |
| Completion handling            | [`coroutine_data_<T>::do_return`](../include/cxxdes/core/impl/environment.ipp#L266) |

## Awaiting

libcxxdes awaitables satisfy the library's `awaitable` concept, which extends the normal coroutine awaiter shape with environment binding and token access.

The common flow is:

1. `await_transform` receives the awaitable.
2. The awaitable is bound to the current environment and inherited priority.
3. `await_ready()` decides whether the current coroutine must suspend.
4. `await_suspend(...)` usually creates and schedules a token.
5. The environment later resumes the waiting coroutine through that token.
6. `await_resume()` produces the result of the `co_await` expression.

See [await_transform.ipp](../include/cxxdes/core/impl/await_transform.ipp), [awaitable.ipp](../include/cxxdes/core/impl/awaitable.ipp), and [barebones_awaitable.cpp](../examples/barebones_awaitable.cpp).

## Awaiting Another Coroutine

A `coroutine<T>` is itself awaitable.
When one coroutine awaits another, the awaited coroutine is bound and started if needed.
If it is not already complete, the waiting coroutine creates a completion token and gives it to the awaited coroutine data.

When the awaited coroutine returns, it schedules its completion tokens.
Those tokens resume the coroutines that were waiting for it.

This is the mechanism behind code like:

```cpp
coroutine<int> child() {
    co_return 4;
}

coroutine<> parent() {
    int value = co_await child();
}
```

Return values are stored in coroutine data.
For `coroutine<T>`, `co_await` returns the stored value; for `unique_coroutine<T>`, the value is moved out.

## Completion Tokens And Composition

Completion tokens are the bridge between ordinary coroutine awaiting and control-flow composition.
When a coroutine awaits another `coroutine<T>`, the waiting coroutine creates a completion token in `coroutine<T>::await_suspend(...)`.
That token points back to the waiting coroutine's `coroutine_data`.
The awaited coroutine stores the token with `coroutine_data::completion_token(...)`.
When the awaited coroutine returns, `coroutine_data::do_return()` schedules its stored completion tokens.

Compositions such as `any_of` and `all_of` use the same token mechanism, but they insert a handler between each child awaitable and the waiting coroutine.
When the child awaitable is a `coroutine<T>`, that child token is the coroutine's completion token.
When a composition suspends, it creates one output token for the coroutine that is awaiting the composition.
It then suspends each child awaitable and installs a shared handler on each child token returned by `await_token()`.
When a child token fires, the environment invokes the handler instead of directly resuming the original coroutine.
The handler counts completions, checks for token exceptions, and schedules the composition's output token when the composition condition is satisfied.

For `any_of`, the condition is satisfied when at least one child completes.
For `all_of`, the condition is satisfied when all children complete.
The output token inherits timing and priority information from the child token that satisfied the condition.
That scheduled output token is what finally resumes the coroutine waiting on the composition.

This is why every libcxxdes awaitable exposes `await_token()`.
Compositions need access to the token that represents each child awaitable's eventual completion.
Awaitables that complete immediately may return `nullptr`, and the composition accounts for them through `await_ready()`.

| Mechanism | Source |
| --- | --- |
| Coroutine completion token creation | [`coroutine<T>::await_suspend`](../include/cxxdes/core/impl/coroutine.ipp#L147) |
| Completion token storage | [`coroutine_data::completion_token`](../include/cxxdes/core/impl/coroutine_data.ipp#L184) |
| Completion token scheduling | [`schedule_completion_`](../include/cxxdes/core/impl/environment.ipp#L245) |
| Composition token handlers | [`any_of.ipp`](../include/cxxdes/core/impl/any_of.ipp) |

## Subroutines

`subroutine<T>` is different from `coroutine<T>`.

A `coroutine<T>` is an independently schedulable process.
A `subroutine<T>` is a coroutine frame that runs inside the currently scheduled process.

When a subroutine is awaited:

1. It is bound to the current `coroutine_data`.
2. Its coroutine handle is pushed onto that data object's call stack with `push_coro_`.
3. The environment still sees one scheduled process.
4. The process resumes the top coroutine handle on the stack.
5. When the subroutine finishes, its `final_suspend()` calls `pop_coro_`.
6. Execution continues in the caller.

This is why subroutines are useful for structured helper code.
They let code await simulation events without creating a separate process in the environment.

See [subroutine.ipp](../include/cxxdes/core/impl/subroutine.ipp), [coroutine_data.ipp](../include/cxxdes/core/impl/coroutine_data.ipp), and [pitfall.cpp](../examples/pitfall.cpp).

## The Coroutine Call Stack

Each `coroutine_data` owns a stack of coroutine handles.
The first handle is the process coroutine, pushed by the process promise constructor before the process is ever bound to an environment.
Awaited subroutines are pushed on top of it.

When the environment resumes a process, `coroutine_data::resume()` resumes the top frame.
The `should_continue_` flag lets execution immediately continue after a subroutine call or return, so nested subroutine calls behave like normal structured function calls from the user's point of view.

This stack is not a C++ call stack.
It is an explicit coroutine-handle stack managed by libcxxdes.

## Parent Links And Introspection

When a coroutine is bound while another coroutine is running, its `coroutine_data` records the current coroutine as its parent.
The await transform path also records source locations.

Together, these allow examples such as [stack.cpp](../examples/stack.cpp) to walk from the current coroutine through parent processes and print where each was awaited.

Useful introspection awaitables:

| Awaitable          | Returns                       |
| ------------------ | ----------------------------- |
| `this_coroutine`   | The current `coroutine_data`. |
| `this_environment` | The current `environment`.    |

## Exceptions

Exceptions thrown inside a `coroutine<T>` are handled by the coroutine promise's `unhandled_exception()` function.
The promise stores the current exception in completion tokens through `coroutine_data::propagate_exception(...)`, then marks the coroutine as returned.
When a waiting coroutine resumes from its completion token, `coroutine<T>::await_resume(...)` rethrows the exception.

In practical terms, an exception thrown by an awaited coroutine is observed at the `co_await` site:

```cpp
try {
    co_await failing_process();
}
catch (std::exception const &ex) {
    // handle the process failure here
}
```

Subroutines use a smaller version of the same idea.
A `subroutine<T>` promise stores an `exception_ptr`, and `subroutine<T>::await_resume()` rethrows it into the caller.

Composition helpers also participate in exception propagation.
For example, `any_of` and `all_of` inspect completed tokens and rethrow token exceptions through their handler.
This makes exceptions from composed awaitables visible to the process awaiting the composition.

One important edge case is `async(...)`: starting a coroutine asynchronously does not automatically mean the caller will observe its exception.
The exception is reported through completion tokens, so code that starts a coroutine with `async(...)` should still await the returned coroutine if it needs to observe failure.

| Mechanism                       | Source                                                                                                                           |
| ------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| Coroutine exception propagation | [coroutine.ipp](../include/cxxdes/core/impl/coroutine.ipp), [coroutine_data.ipp](../include/cxxdes/core/impl/coroutine_data.ipp) |
| Token-level exception storage   | [token.ipp](../include/cxxdes/core/impl/token.ipp)                                                                               |
| Environment rethrow path        | [environment.ipp](../include/cxxdes/core/impl/environment.ipp)                                                                   |
| Subroutine exception storage    | [subroutine.ipp](../include/cxxdes/core/impl/subroutine.ipp)                                                                     |
| Composition exception handling  | [any_of.ipp](../include/cxxdes/core/impl/any_of.ipp)                                                                             |
| Example coverage                | [exceptions.cpp](../examples/exceptions.cpp)                                                                                     |

## Practical Rules

- Use `coroutine<T>` for simulation processes that can be scheduled, composed, or awaited independently.
- Use `subroutine<T>` for helper logic that should run as part of the current process.
- Convert a subroutine with `.as_coroutine()` when it must participate in process composition.
- Be careful when composing by reference; the referenced awaitable must outlive the composition.
- Be careful when a coroutine method uses `this`; the object must outlive the coroutine.
- Await asynchronously started coroutines if their exceptions must be observed.

Examples:

- [pitfall.cpp](../examples/pitfall.cpp)
- [this_out_scope.cpp](../examples/this_out_scope.cpp)
- [exceptions.cpp](../examples/exceptions.cpp)
