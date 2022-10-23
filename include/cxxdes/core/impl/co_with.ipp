namespace cxxdes {
namespace core {

template <typename T>
concept releasable = requires(T t) {
    { t.release() } -> awaitable;
};

template <typename T>
concept acquirable = requires(T t) {
    { t.acquire() } -> awaitable;
    { t.acquire().await_resume() } -> releasable;
};

} /* namespace core */
} /* namespace cxxdes */


template <cxxdes::core::acquirable A, typename F>
cxxdes::core::coroutine<void> operator+(A &a, F &&f) {
    auto handle = co_await a.acquire();
    co_await std::forward<F>(f)();
    co_await handle.release();
}

#define _Co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::coroutine<void>
