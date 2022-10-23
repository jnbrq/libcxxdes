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
#include <type_traits>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cxxdes/core/core.hpp>
#include <cxxdes/misc/utils.hpp>

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
        std::size_t total = 0;
        std::size_t remaining = 0;
        memory::ptr<token> completion_tkn = nullptr;
        environment *env = nullptr;

        void invoke(token *tkn) override {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            --remaining;
            
            if (completion_tkn && Condition::operator()(total, remaining)) {
                // inherit the output_event features
                completion_tkn->time += tkn->time;
                completion_tkn->priority = tkn->priority;
                completion_tkn->phandle = tkn->phandle;
                env->schedule_token(completion_tkn.get());
                completion_tkn = nullptr;
            }
        }
    };

    template <typename Derived>
    struct base: Condition {
        auto &priority(priority_type priority) noexcept {
            priority_ = priority;
            return *this;
        }

        auto &return_latency(time_integral latency) noexcept {
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

        void await_suspend(coroutine_info_ptr phandle) {
            CXXDES_DEBUG_MEMBER_FUNCTION;

            tkn_ = new token(latency_, priority_, phandle);

            auto handler = new custom_handler;
            handler->total = derived().count();
            handler->remaining = remaining_;
            handler->env = env_;
            handler->completion_tkn = tkn_;

            derived().apply([&](auto &a) {
                a.await_suspend(phandle);
                if (a.await_token())
                    a.await_token()->handler = handler;
            });
        }

        token *await_token() const noexcept {
            return tkn_;
        }

        void await_resume(no_return_value_tag = {}) {
            CXXDES_DEBUG_MEMBER_FUNCTION;
            derived().apply([&](auto &a) { a.await_resume(no_return_value_tag{}); });
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
        time_integral latency_ = 0;
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

        range_based(Iterator first_, Iterator last_, std::size_t size_):
            first{first_}, last{last_}, size{size_} {
            // see the note below
        }
        
        constexpr std::size_t count() const noexcept {
            return size;
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::for_each(first, last, f);
        }
    };

    template <typename ValueType>
    struct vector_based: base<vector_based<ValueType>> {
        std::vector<ValueType> v;

        template <typename Iterator>
        vector_based(Iterator begin, Iterator end): v(begin, end) {
        }
        
        constexpr std::size_t count() const noexcept {
            return v.size();
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::for_each(v.begin(), v.end(), f);
        }
    };

    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await any_of(awaitables...) or all_of(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return by_value(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.by_value(awaitables...)")]]
        constexpr auto by_value(Ts && ...ts) const {
            return tuple_based<std::remove_cvref_t<Ts>...>{ std::forward<Ts>(ts)... };
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.by_reference(awaitables...)")]]
        constexpr auto by_reference(Ts && ...ts) const {
            return tuple_based<Ts &&...>{ std::forward<Ts>(ts)... };
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.copy_rvalues(awaitables...)")]]
        constexpr auto copy_rvalues(Ts && ...ts) const {
            return tuple_based<Ts...>{ std::forward<Ts>(ts)... };
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return range_copy(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range_copy(begin, end)")]]
        constexpr auto range_copy(Iterator first, Iterator last) const {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            return vector_based<value_type>(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range_no_copy(begin, end)")]]
        constexpr auto range_no_copy(Iterator first, Iterator last) const {
            // -Werror=missing-field-initializers
            // we cannot use the following due to a GCC bug
            /*
            return range_based<Iterator>{
                .first = first,
                .last = last,
                .size = (std::size_t) std::distance(first, last)
            };
            */
            return range_based<Iterator>(first, last, (std::size_t) std::distance(first, last));
        }
    };
};

struct any_of_condition {
    constexpr bool operator()(std::size_t total, std::size_t remaining) const {
        return remaining < total;
    }
};

struct all_of_condition {
    constexpr bool operator()(std::size_t /* total */, std::size_t remaining) const {
        return remaining == 0;
    }
};

inline constexpr any_all_helper<any_of_condition>::functor any_of;
inline constexpr any_all_helper<all_of_condition>::functor all_of;

struct sequential_helper {
    template <typename ...Ts>
    static coroutine<void> seq_proc_tuple(Ts ...ts) {
        ((co_await std::move(ts)), ...);
        co_return ;
    }

    template <typename Iterator>
    static coroutine<void> seq_proc_range(Iterator begin, Iterator end) {
        for (Iterator it = begin; it != end; ++it)
            co_await (*it);
        co_return;
    }

    template <typename ValueType>
    static coroutine<void> seq_proc_vector(std::vector<ValueType> v) {
        for (auto &a: v)
            co_await std::move(a);
        co_return;
    }
    
    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return by_value(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.by_value(awaitables...)")]]
        constexpr auto by_value(Ts && ...ts) const {
            return seq_proc_tuple<std::remove_cvref_t<Ts>...>(std::forward<Ts>(ts)... );
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.by_reference(awaitables...)")]]
        constexpr auto by_reference(Ts && ...ts) const {
            return seq_proc_tuple<Ts &&...>(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.copy_rvalues(awaitables...)")]]
        constexpr auto copy_rvalues(Ts && ...ts) const {
            return seq_proc_tuple<Ts...>(std::forward<Ts>(ts)...);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return range_copy(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range_copy(begin, end)")]]
        constexpr auto range_copy(Iterator first, Iterator last) const {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            return seq_proc_vector(std::vector<value_type>(first, last));
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range_no_copy(begin, end)")]]
        constexpr auto range_no_copy(Iterator first, Iterator last) const {
            return seq_proc_range(first, last);
        }
    };
};

inline constexpr sequential_helper::functor sequential;

struct async_functor {
    template <typename T>
    [[nodiscard("expected usage: co_await async(coroutine<T>)")]]
    constexpr auto operator()(coroutine<T> p) const {
        // since coroutine<T> is a reference-counted object with a flexible
        // lifetime, we can safely use coroutine<R> with async.
        // for other types of awaitables, they should be wrapped in
        // a coroutine to be used with async.
        struct async_awaitable {
            coroutine<T> p;

            void await_bind(environment *env, priority_type priority) {
                p.await_bind(env, priority);
            }

            bool await_ready() {
                return true;
            }

            void await_suspend(coroutine_info_ptr) const noexcept {
            }

            token *await_token() const noexcept {
                return nullptr;
            }

            auto await_resume() const noexcept {
                return p;
            }

            void await_resume(no_return_value_tag) const noexcept {  }
        };

        return async_awaitable{p};
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async(awaitable)")]]
    constexpr auto operator()(A &&a) const {
        // We need to wrap the awaitable in a coroutine to support async.
        // There probably is not a good use case for this.
        return by_value(std::forward<A>(a));
    }
    
    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_value(awaitable)")]]
    constexpr auto by_value(A &&a) const {
        return (*this)(sequential.by_value(std::forward<A>(a)));
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_reference(awaitable)")]]
    constexpr auto by_reference(A &&a) const {
        return (*this)(sequential.by_reference(std::forward<A>(a)));
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async.copy_rvalue(awaitable)")]]
    constexpr auto copy_rvalue(A &&a) const {
        return (*this)(sequential.copy_rvalues(std::forward<A>(a)));
    }
};

inline constexpr async_functor async;

} /* namespace detail */

using detail::any_of;
using detail::all_of;
using detail::sequential;
using detail::async;

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
auto operator>>(A &&a, Output &output) -> coroutine<void> {
    output = co_await a;
}

auto flag_done(bool &flag) -> coroutine<void> {
    flag = true;
    co_return ;
}

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_COMPOSITIONS
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_COMPOSITIONS_HPP_INCLUDED */
