# Awaitables

A type `T` and objects of type `T` are said to be _awaitable_ if `T` satisfies the requirements of the `cxxdes::core::awaitable` concept defined in `include/cxxdes/core/awaitable.hpp` as the following:

```cpp
namespace cxxdes {
namespace core {

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type inherited_priority,
    coroutine_handle current_coro) {
    { t.await_bind(env, inherited_priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(current_coro) };
    { t.await_token() } -> std::same_as<token *>;
    { t.await_resume() };
};

} /* namespace core */
} /* namespace cxxdes */
```

Functions `bool T::await_ready()`, `void T::await_suspend()` and `void T::await_resume(coroutine_handle)` are the requirements of awaitables as defined by [the C++ standard](https://en.cppreference.com/w/cpp/language/coroutines), whereas `void T::await_bind(environment *, priority_type)` and `token *await_token()` are required by `libcxxdes`.
These functions are called in a sequence throughout the lifetime of an awaitable object.

## Lifetime

The lifetime of an awaitable starts as soon as its constructor is called. Initially, the awaitable is not bound to an `environment`.
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

### Lifetimes of the Promise and Coroutine Objects

Coroutine objects, such as `foo()` in the example above, outlive the [promise](https://en.cppreference.com/w/cpp/language/coroutines#Execution) objects. Therefore, if a coroutine returns information back to the executor of the `co_await` expression, the return value must live as long as the coroutine object, not the promise object. Hence, return values should be stored by value inside the coroutine objects.

## Awaitable Execution

### await_bind

As the first step of awaitable execution, `void T::await_bind(environment *, priority_type)` is called. This function passes the context to the awaitable object, for example, the environment parameter enables scheduling tokens. This function also takes a priority variable, in case the awaitable object needs to inherit priorities from the calling coroutine.

`await_bind()` is usually called by `<awaitable A> auto &&promise_type::await_transform(A &&a)` (defined in `include/cxxdes/core/coroutine.hpp`). `await_transform` is also used to implement the `this_coroutine` API, please check `include/cxxdes/core/coroutine.hpp` for details.

In the later versions of the `libcxxdes`, additional information may be passed using the `await_bind` function. For example, it can be used to implement recoding stack traces for debugging.

### await_ready

`bool T::await_ready()` determines whether the current coroutine should be suspended or not (it returns `false` if suspended). In most cases, `await_ready` returns `false`, that is, the control flow passes to another coroutine.

`template <typename T> struct immediately_returning_awaitable` is an exceptional case which `await_ready()` returns `true`. In `libcxxdes`, `immediately_returning_awaitable` is used to return information regarding the current coroutine (like its inheritance priority, return priority and environment), **without interrupting the control flow**. Please see `examples/await_transform_extender.cpp` for an example use.

### await_suspend

`void T::await_suspend(coroutine_handle)` is called in case the current coroutine is to be suspended. It usually creates a `token` object (with `new token{...}`) to resume the current coroutine, and it might also schedule it (as in the case of `cxxdes::core::timeout`). The newly created `token` object should be stored as part of the awaitable object to be returned later by `await_token`.

### await_token

`token *T::await_token()` returns the `token` object created by `await_suspend` to resume the current coroutine. For compositions, such as `any_of` and `all_of`, this token is modified to implement the desired functionality. See `include/cxxdes/core/compositions.hpp` for details.

### await_resume

`R T::await_resume()` is called when the coroutine that the awaitable had suspended is resumed. The return value of this function is the result of the `co_await` expression. The lifetime of the coroutine object ends after this point; therefore, `await_resume` should not return a reference to an object inside the coroutine object.

`await_resume` is not called in case the return value cannot be returned. For example, compositions do not call `await_resume` of the wrapped handlers.
