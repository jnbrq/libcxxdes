namespace cxxdes {
namespace core {
namespace detail {

struct under_helper {  };

} /* namespace detail */
} /* namespace core */
} /* namespace cxxdes */

template <typename F>
auto operator+(cxxdes::core::detail::under_helper, F f) {
    return f();
}

/** @brief Creates an inline anonymous `coroutine<void>` from a lambda body. */
#define _Coroutine(...) cxxdes::core::detail::under_helper{} + [&](__VA_ARGS__) -> coroutine<void>
