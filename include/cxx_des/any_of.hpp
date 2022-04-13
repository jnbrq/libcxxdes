/**
 * @file any_of.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Any-of operator ||.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_ANY_OF_HPP_INCLUDED
#define CXX_DES_ANY_OF_HPP_INCLUDED

#include "process.hpp"

// TODO write any_of and all_of.

namespace cxx_des {

namespace detail {

template <typename ...T>
struct any_of_base;

template <typename ...A>
struct any_of_base<wrap_awaitable<A>...> {
    std::tuple<wrap_awaitable<A>...> was;

    event *on_suspend(process::promise_type &promise, std::coroutine_handle<> handle) {

    }

    void on_resume() {

    }
};

template <typename ...T>
using any_of_type = wrap_awaitable<any_of_base<T...>>;

}

template <typename ...A>
auto any_of(wrap_awaitable<A> && ...was) {
    return detail::any_of_type<wrap_awaitable<A>...>{ std::make_tuple(was...) };
}

}


#endif /* CXX_DES_ANY_OF_HPP_INCLUDED */
