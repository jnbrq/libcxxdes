# Awaitables

[README](../README.md) | [Documentation index: Awaitables](index.md#awaitables)

A type `T` is a libcxxdes awaitable if it satisfies the `cxxdes::core::awaitable` concept.
The concept is defined in [awaitable.ipp](../include/cxxdes/core/impl/awaitable.ipp) and is included through [core.hpp](../include/cxxdes/core/core.hpp).

```cpp
namespace cxxdes {
namespace core {

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type inherited_priority,
    coroutine_data_ptr coro_data) {
    { t.await_bind(env, inherited_priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(coro_data) };
    { t.await_token() } -> std::same_as<token *>;
    { t.await_resume() };
    { t.await_resume(no_return_value_tag {}) } -> std::same_as<void>;
};

} /* namespace core */
} /* namespace cxxdes */
```

The functions `await_ready`, `await_suspend`, and `await_resume` correspond to the usual C++ coroutine awaiter operations.
The functions `await_bind` and `await_token` are libcxxdes-specific.
The overload `await_resume(no_return_value_tag)` lets compositions resume or clean up an awaitable without consuming its normal return value.
These functions are called in a sequence throughout the lifetime of an awaitable object.

## Lifetime

The lifetime of an awaitable starts as soon as its constructor is called.
Initially, the awaitable is not bound to an `environment`.
`co_await` is the most common mechanism that triggers the interaction between an environment and an awaitable:

```cpp

// f is a function that returns an awaitable object
auto f() -> some_awaitable { /* ... */ }

co_await f();
```

In general, as soon as the `co_await` expression starts to execute, the control flow of the simulation jumps to another coroutine.
[The C++ standard](https://en.cppreference.com/w/cpp/language/lifetime#Temporary_object_lifetime) guarantees that the temporary object `f()` is alive until the end of the evaluation of the `co_await` expression.
That is, `f()` is alive even if the control flow passes to another coroutine.

Furthermore, all the temporary objects created as part of the `co_await` expression are kept alive (even other awaitables):

```cpp
// coroutine<T> is awaitable

coroutine<int> foo() {
    co_return 4;
}

coroutine<int> bar(coroutine<int> &&other) {
    auto r = co_await other;
    std::cout << r << "\n";
    co_return r;
}

// ...

co_await bar(foo());
//           ^ foo() is alive until the end of the co_await expression
//           ^ no dangling references

```

In any case, `libcxxdes` is subject to the lifetime rules for coroutines; therefore, it is advisable to get familiar with [them](https://en.cppreference.com/w/cpp/language/coroutines#Execution).

Some awaitables also depend on an external synchronization object.
For the lifetime rules of `event`, `semaphore`, `queue`, `mutex`, and `resource`, see [sync_primitives.md](sync_primitives.md#lifetime-rule).

### Lifetimes of the Promise and Coroutine Objects

Coroutine objects, such as `foo()` in the example above, outlive the [promise](https://en.cppreference.com/w/cpp/language/coroutines#Execution) objects.
Therefore, if a coroutine returns information back to the executor of the `co_await` expression, the return value must live as long as the coroutine object, not the promise object.
Return values should be stored by value inside the coroutine state.

## Awaitable Execution

### await_bind

As the first step of awaitable execution, `void T::await_bind(environment *, priority_type)` is called.
This function passes the current simulation context to the awaitable object.
For example, the environment parameter enables the awaitable to schedule tokens.
The priority parameter lets the awaitable inherit priority from the calling coroutine when appropriate.

`await_bind()` is usually called by `promise_type::await_transform(...)`, defined in [await_transform.ipp](../include/cxxdes/core/impl/await_transform.ipp).
`await_transform` is also used to implement `this_coroutine`, `this_environment`, and extension hooks.

Additional context may be passed through this path in future versions.
For example, the current implementation already records source locations for introspection.

### await_ready

`bool T::await_ready()` determines whether the current coroutine should suspend.
It returns `false` when the coroutine should suspend.
Most scheduling awaitables return `false`.

`immediately_return` is an exception to that rule.
It returns information such as the current coroutine or environment without interrupting control flow.
See [await_transform_extender.cpp](../examples/await_transform_extender.cpp) for an example.

### await_suspend

`void T::await_suspend(coroutine_data_ptr)` is called when the current coroutine should suspend.
It usually creates a `token` object that can resume the current coroutine later.
It may also schedule that token immediately, as `cxxdes::core::timeout` does.
The newly created token should be stored by the awaitable so `await_token()` can return it later.

## A Minimal Timeout Awaitable

The built-in timeout awaitables are defined in [timeout.ipp](../include/cxxdes/core/impl/timeout.ipp).
A simplified relative timeout has the same basic shape:

```cpp
struct simple_timeout {
    time_integral delay;
    priority_type priority = priority_consts::inherit;

    environment *env = nullptr;
    token *tkn = nullptr;

    void await_bind(environment *e, priority_type inherited_priority) noexcept {
        env = e;

        if (priority == priority_consts::inherit) {
            priority = inherited_priority;
        }
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coroutine_data_ptr coro_data) {
        tkn = new token(
            env->now() + delay,
            priority,
            coro_data,
            "simple timeout"
        );

        env->schedule_token(tkn);
    }

    token *await_token() const noexcept {
        return tkn;
    }

    void await_resume(no_return_value_tag = {}) noexcept {
        tkn = nullptr;
    }
};
```

The important steps are:

1. `await_bind(...)` receives the current `environment` and inherited priority.
2. `await_ready()` returns `false`, so the current coroutine suspends.
3. `await_suspend(...)` creates a token scheduled at `env->now() + delay`.
4. `env->schedule_token(...)` inserts that token into the environment's event queue.
5. `await_token()` exposes the token to composition helpers such as `any_of` and `all_of`.
6. `await_resume(...)` runs when the environment later resumes the coroutine from that token.

The production implementation generalizes this pattern for relative timeouts, absolute deadlines, real-time expressions, lazy deadlines, inherited priorities, and `yield()`.

### await_token

`token *T::await_token()` returns the token created by `await_suspend`.
Compositions such as `any_of` and `all_of` inspect or modify these tokens to implement control-flow behavior.
See [compositions.ipp](../include/cxxdes/core/impl/compositions.ipp) and [any_of.ipp](../include/cxxdes/core/impl/any_of.ipp).

### await_resume

`R T::await_resume()` is called when the coroutine that the awaitable suspended is resumed.
The return value of this function is the result of the `co_await` expression.
The awaitable may be destroyed after the `co_await` expression completes, so `await_resume` should not return a reference to state owned only by the awaitable.

`void T::await_resume(no_return_value_tag)` is used when the caller needs completion behavior without consuming the normal return value.
For example, compositions use this path for wrapped awaitables.
