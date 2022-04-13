/**
 * @file any_all_of.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief operator|| and operator&& for awaitables.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_ANY_ALL_OF_HPP_INCLUDED
#define CXX_DES_ANY_ALL_OF_HPP_INCLUDED

#include <tuple>

#include "process.hpp"
#include "environment.hpp"

namespace cxx_des {

namespace detail {
namespace any_all_of {

template <typename Condition>
struct giant {
    struct modified_handler: event_handler, Condition {
        struct shared_data_type {
            bool done;
            std::size_t remaining;
            event *output_event;
            environment *env;
        };

        // to be restored
        event_handler *old_handler;
        bool old_cleanup_handler;

        shared_data_type *shared_data;

        void invoke(event *evt) override {
            --(shared_data->remaining);
            
            if (!shared_data->done) {
                if (Condition::operator()(shared_data->remaining)) {
                    // inherit the output_event features
                    shared_data->output_event->time = evt->time;
                    shared_data->output_event->priority = 1; //evt->priority;
                    shared_data->output_event->coroutine_handle = evt->coroutine_handle;
                    shared_data->output_event->handler = old_handler; // overwrite the event handler modified by the outer operator
                    shared_data->output_event->cleanup_handler = old_cleanup_handler;

                    old_cleanup_handler = false; // do not destroy in the destructor

                    shared_data->env->append_event(shared_data->output_event);

                    shared_data->done = true;
                }
            }

            if (shared_data->remaining == 0) {
                delete shared_data;
            }
        }

        ~modified_handler() override {
            if (old_cleanup_handler)
                delete old_handler;
        }
    };

    template <awaitable ...As>
    struct result_type: std::tuple<As...> {
        using std::tuple<As...>::tuple;

        event *on_suspend(process::promise_type promise, std::coroutine_handle<> coroutine_handle) {
            auto output_event = new event{0, 0, coroutine_handle};
            auto shared_data = new typename modified_handler::shared_data_type{ false, sizeof...(As), output_event, promise.env };

            // output_event shall not allocate memory for handler by default

            auto overwrite = [&](event *evt) {
                auto handler = new modified_handler;
                handler->old_handler = evt->handler;
                handler->old_cleanup_handler = evt->cleanup_handler;
                handler->shared_data = shared_data;

                evt->handler = handler;
                evt->cleanup_handler = true;
            };

            std::apply([&](As & ...as) { (overwrite(as.on_suspend(promise, coroutine_handle)), ...); }, (std::tuple<As...> &)(*this));
            return output_event;
        }

        void on_resume() {
            // call on_resume on each awaitable
            std::apply([](As & ...as) { (as.on_resume(), ...); }, (std::tuple<As...> &)(*this));
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

template <typename ...Ts>
auto any_of(Ts && ...ts) {
    return giant<any_of_condition>::result_type<std::unwrap_ref_decay_t<Ts>...>{ std::forward<Ts>(ts)... };
}

template <typename ...Ts>
auto all_of(Ts && ...ts) {
    return giant<all_of_condition>::result_type<std::unwrap_ref_decay_t<Ts>...>{ std::forward<Ts>(ts)... };
}

} // namespace any_all_of
} // namespace detail

using detail::any_all_of::any_of;
using detail::any_all_of::all_of;

template <awaitable A1, awaitable A2>
auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::move(a1), std::move(a2));
}

template <awaitable A1, awaitable A2>
auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::move(a1), std::move(a2));
}

} // namespace cxx_des


#endif /* CXX_DES_ANY_ALL_OF_HPP_INCLUDED */
