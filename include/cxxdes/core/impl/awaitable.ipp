/** @brief Tag used to resume an awaitable without consuming its normal result. */
struct no_return_value_tag {  };

/**
 * @brief Concept implemented by awaitables that participate in libcxxdes scheduling.
 *
 * In addition to the usual coroutine awaiter operations, a libcxxdes awaitable
 * must accept an environment through `await_bind()` and expose the token created
 * by `await_suspend()` through `await_token()`.
 */
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
