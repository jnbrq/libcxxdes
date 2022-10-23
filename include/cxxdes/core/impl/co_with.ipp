namespace cxxdes {
namespace core {

template <typename T>
concept releasable = requires(T t) {
    { t.release().await_ready() } -> std::same_as<bool>;
};

template <typename T>
concept acquirable = requires(T t) {
    { t.acquire().await_ready() } -> std::same_as<bool>;
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

#define _Co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::subroutine<void>
