namespace detail {

#include "any_of.ipp"
#include "sequential.ipp"
#include "async.ipp"

} /* namespace detail */

using detail::any_of;
using detail::all_of;
using detail::sequential;
using detail::async;

template <awaitable A1, awaitable A2>
inline auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
inline auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
inline auto operator,(A1 &&a1, A2 &&a2) {
    return sequential(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A, typename Output>
inline auto operator>>(A &&a, Output &output) -> coroutine<void> {
    output = co_await a;
}

inline auto flag_done(bool &flag) -> coroutine<void> {
    flag = true;
    co_return ;
}
