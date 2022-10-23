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

#define _Coroutine(...) cxxdes::core::detail::under_helper{} + [&](__VA_ARGS__) -> coroutine<void>
