/**
 * @file compositions.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief operator||, operator&& and operator, for awaitables.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_COMPOSITIONS_HPP_INCLUDED
#define CXX_DES_COMPOSITIONS_HPP_INCLUDED

#include <tuple>

#include "process.hpp"
#include "environment.hpp"

namespace cxx_des {

namespace detail {
namespace ns_compositions {

template <typename Condition>
struct giant1 {
    struct new_handler: event_handler, Condition {
        bool done = false;
        std::size_t remaining = 0;
        event *output_event = nullptr;
        environment *env = nullptr;

        void invoke(event *evt) override {
            --remaining;

            // we do not want to be deleted while in use
            evt->handler = nullptr;
            
            if (!done) {
                if (Condition::operator()(remaining)) {
                    // inherit the output_event features
                    output_event->time = evt->time;
                    output_event->priority = evt->priority; //evt->priority;
                    output_event->coroutine_handle = evt->coroutine_handle;
                    env->append_event(output_event);
                    done = true;
                }
            }

            if (remaining == 0) {
                evt->handler = this;
            }
        }
    };

    template <awaitable ...As>
    struct result_type: std::tuple<As...> {
        using std::tuple<As...>::tuple;

        event *on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle) {
            auto handler = new new_handler;
            handler->done = false;
            handler->remaining = sizeof...(As);
            handler->output_event = new event{0, 0, coroutine_handle};
            handler->env = promise->env;
            std::apply([&](As & ...as) { ((as.on_suspend(promise, coroutine_handle)->handler = handler), ...); }, (std::tuple<As...> &)(*this));
            return handler->output_event;
        }

        void on_resume() {
            // call on_resume on each awaitable
            std::apply([](As & ...as) { (as.on_resume(), ...); }, (std::tuple<As...> &)(*this));
        }
    };

    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await any_of(awaitables...) or all_of(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return result_type<std::unwrap_ref_decay_t<Ts>...>{ std::forward<Ts>(ts)... };
        }
    };

    // GCC complains that a deduction guide should be declared in the namespace scope
    // Clang complains that you cannot have a deduction guide for using statements
    // I am very confused :(
    /*
    template <typename ...T>
    result_type(T && ...) -> result_type<std::unwrap_ref_decay_t<T>...>;
    */
};

struct any_of_condition {
    constexpr bool operator()(std::size_t remaining) const {
        return true;
    }
};

struct all_of_condition {
    constexpr bool operator()(std::size_t remaining) const {
        return remaining == 0;
    }
};

constexpr giant1<any_of_condition>::functor any_of;
constexpr giant1<all_of_condition>::functor all_of;

struct giant2 {
    template <awaitable ...As>
    struct result_type: std::tuple<As...> {
        using std::tuple<As...>::tuple;

        struct new_handler {

        };

        event *on_suspend(process::promise_type *promise, std::coroutine_handle<> coroutine_handle) {

        }

        void on_resume() {
            // call on_resume on each awaitable
            std::apply([](As & ...as) { (as.on_resume(), ...); }, (std::tuple<As...> &)(*this));
        }
    };
};

template <typename ...Ts>
auto sequential(Ts && ...ts) {
    return giant2::result_type<std::unwrap_ref_decay_t<Ts>...>{ std::forward<Ts>(ts)... };
}

} // namespace ns_compositions
} // namespace detail

using detail::ns_compositions::any_of;
using detail::ns_compositions::all_of;

template <awaitable A1, awaitable A2>
auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::move(a1), std::move(a2));
}

template <awaitable A1, awaitable A2>
auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::move(a1), std::move(a2));
}

} // namespace cxx_des


#endif /* CXX_DES_COMPOSITIONS_HPP_INCLUDED */
