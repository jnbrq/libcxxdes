/**
 * @file compositions.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief operator||, operator&& and operator, for awaitables.
 * Also, operator>> for capturing return values.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED
#define CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED

#include <tuple>
#include <iterator>
#include <cxxdes/core/process.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_COMPOSITIONS
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

namespace detail {

template <typename Condition>
struct any_all_helper {
    struct custom_handler: token_handler, Condition {
        bool done = false;
        std::size_t total = 0;
        std::size_t remaining = 0;
        token *completion_tkn = nullptr;
        environment *env = nullptr;

        void invoke(token *tkn) override {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            --remaining;

            // do not delete the handler while still in use
            tkn->handler = nullptr;
            
            if (!done) {
                if (Condition::operator()(total, remaining)) {
                    // inherit the output_event features
                    completion_tkn->time += tkn->time;
                    completion_tkn->priority = tkn->priority;
                    completion_tkn->coro = tkn->coro;
                    env->schedule_token(completion_tkn);
                    done = true;
                }
            }

            if (remaining == 0) {
                // delete the handler
                tkn->handler = this;
            }
        }
    };

    template <typename Derived>
    struct base: Condition {
        auto &priority(priority_type priority) noexcept {
            priority_ = priority;
            return *this;
        }

        auto &return_latency(time_type latency) noexcept {
            latency_ = latency;
            return *this;
        }

        void await_bind(environment *env, priority_type priority) {
            CXXDES_DEBUG_MEMBER_FUNCTION;
            
            env_ = env;

            if (priority_ == priority_consts::inherit)
                priority_ = priority;
            
            derived().apply([&](auto &a) { a.await_bind(env, priority_); });
        }

        bool await_ready() {
            auto total = derived().count();
            remaining_ = total;
            derived().apply([&](auto &a) mutable { remaining_ -= (a.await_ready() ? 1 : 0); });
            return Condition::operator()(total, remaining_);
        }

        void await_suspend(coro_handle current_coro) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            tkn_ = new token(latency_, priority_, current_coro);

            auto handler = new custom_handler;
            handler->total = derived().count();
            handler->remaining = remaining_;
            handler->env = env_;
            handler->completion_tkn = tkn_;

            derived().apply([&](auto &a) {
                a.await_suspend(current_coro);
                if (a.await_token())
                    a.await_token()->handler = handler;
            });
        }

        token *await_token() const noexcept {
            return tkn_;
        }

        void await_resume() {
            CXXDES_DEBUG_MEMBER_FUNCTION;
            
            // As we cannot return a value from compositions, no need call await_resume()
            // derived().apply([&](auto &a) { a.await_resume(); });
        }

    private:
        std::size_t remaining_ = 0;

        auto derived() -> auto & {
            return *(static_cast<Derived *>(this));
        }

        auto derived() const -> auto const & {
            return *(static_cast<Derived const *>(this));
        }
        
        environment *env_ = nullptr;
        token *tkn_ = nullptr;
        time_type latency_ = 0;
        priority_type priority_ = priority_consts::inherit;
    };

    template <awaitable ...As>
    struct tuple_based: std::tuple<As...>, base<tuple_based<As...>> {
        using std::tuple<As...>::tuple;
        
        constexpr std::size_t count() const noexcept {
            return sizeof...(As);
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::apply([&](As & ...as) { (f(as), ...); }, (std::tuple<As...> &)(*this));
        }
    };

    template <typename Iterator>
    struct range_based: base<range_based<Iterator>> {
        Iterator first, last;
        std::size_t size;
        
        constexpr std::size_t count() const noexcept {
            return size;
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::for_each(first, last, f);
        }
    };

    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await any_of(awaitables...) or all_of(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return tuple_based<Ts...>{ std::forward<Ts>(ts)... };
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await any_of.range(begin, end) or all_of.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return range_based<Iterator>{
                .first = first,
                .last = last,
                .size = (std::size_t) std::distance(first, last)
            };
        }
    };
};

struct any_of_condition {
    constexpr bool operator()(std::size_t total, std::size_t remaining) const {
        return remaining < total;
    }
};

struct all_of_condition {
    constexpr bool operator()(std::size_t total, std::size_t remaining) const {
        return remaining == 0;
    }
};

constexpr any_all_helper<any_of_condition>::functor any_of;
constexpr any_all_helper<all_of_condition>::functor all_of;

struct sequential_helper {
    template <typename ...Ts>
    static process<void> seq_proc_tuple(Ts ...ts) {
        ((co_await ts), ...);
        co_return ;
    }

    template <typename Iterator>
    static process<void> seq_proc_range(Iterator begin, Iterator end) {
        for (Iterator it = begin; it != end; ++it) {
            co_await (*it);
        }
        co_return;
    }
    
    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return seq_proc_tuple<Ts...>(std::forward<Ts>(ts)...);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return seq_proc_range(first, last);
        }
    };
};

constexpr sequential_helper::functor sequential;

} /* namespace detail */

using detail::any_of;
using detail::all_of;
using detail::sequential;

template <awaitable A1, awaitable A2>
auto operator||(A1 &&a1, A2 &&a2) {
    return any_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
auto operator&&(A1 &&a1, A2 &&a2) {
    return all_of(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A1, awaitable A2>
auto operator,(A1 &&a1, A2 &&a2) {
    return sequential(std::forward<A1>(a1), std::forward<A2>(a2));
}

template <awaitable A, typename Output>
auto operator>>(A &&a, Output &output) -> process<void> {
    output = co_await a;
}

auto flag_done(bool &flag) -> process<void> {
    flag = true;
    co_return ;
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_COMPOSITIONS
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED */
