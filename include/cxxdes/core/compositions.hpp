/**
 * @file compositions.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief operator||, operator&& and operator, for awaitables.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED
#define CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED

#include <tuple>

#include <cxxdes/core/process.hpp>
#include <cxxdes/core/environment.hpp>

namespace cxxdes {
namespace core {

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
                    output_event->priority = evt->priority;
                    output_event->coro = evt->coro;
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

        event *on_suspend(promise_base *promise, coro_handle coro) {
            auto handler = new new_handler;
            handler->done = false;
            handler->remaining = sizeof...(As);
            handler->output_event = new event{0, 0, coro};
            handler->env = promise->env;
            std::apply([&](As & ...as) { ((as.on_suspend(promise, coro)->handler = handler), ...); }, (std::tuple<As...> &)(*this));
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

        static process<> p(environment *env, As & ...as) {
            ((co_await as), ...);
            co_return ;
        }
        
        event *on_suspend(promise_base *promise, coro_handle coro) {
            event *output_event = new event{ 0, 1000, coro };
            auto pp = std::apply([&](As & ...as) { return p(promise->env, as...); }, (std::tuple<As...> &)(*this));
            pp.this_promise()->completion_evt = output_event;
            pp.start(*(promise->env));
            return output_event;
        }

        void on_resume() {
            // call on_resume on each awaitable
            std::apply([](As & ...as) { (as.on_resume(), ...); }, (std::tuple<As...> &)(*this));
        }
    };

    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return result_type<std::unwrap_ref_decay_t<Ts>...>{ std::forward<Ts>(ts)... };
        }
    };
};

constexpr giant2::functor sequential;


} /* namespace ns_compositions */
} /* namespace detail */

using detail::ns_compositions::any_of;
using detail::ns_compositions::all_of;
using detail::ns_compositions::sequential;

template <awaitable A1, awaitable A2>
auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::move(a1), std::move(a2));
}

template <awaitable A1, awaitable A2>
auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::move(a1), std::move(a2));
}

template <awaitable A1, awaitable A2>
auto operator,(A1 &&a1, A2 &&a2) {
    return sequential(std::move(a1), std::move(a2));
}

} /* namespace core */
} /* namespace cxxdes */


#endif /* CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED */
