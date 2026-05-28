namespace cxxdes {
namespace core {

/** @brief Concept for handles that can be released by awaiting `release()`. */
template <typename T>
concept releasable = requires(T t) {
    { t.release().await_ready() } -> std::same_as<bool>;
};

/** @brief Concept for objects that can be acquired into a releasable handle. */
template <typename T>
concept acquirable = requires(T t) {
    { t.acquire().await_ready() } -> std::same_as<bool>;
    { t.acquire().await_resume() } -> releasable;
};

} /* namespace core */
} /* namespace cxxdes */


/**
 * @brief Acquires @p a, runs @p f, then releases the acquired handle.
 *
 * This helper underlies the `_Co_with(x) { ... }` macro. If the body throws,
 * this implementation does not provide scope-exit release semantics.
 */
template <cxxdes::core::acquirable A, typename F>
cxxdes::core::coroutine<void> operator+(A &a, F &&f) {
    auto handle = co_await a.acquire();
    co_await std::forward<F>(f)();
    co_await handle.release();
}

/** @brief Starts a coroutine body with automatic acquire/body/release syntax. */
#define _Co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::subroutine<void>
