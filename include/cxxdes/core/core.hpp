/**
 * @file core.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief coroutine and environment.
 * @date 2022-10-20
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_CORE_HPP_INCLUDED
#define CXXDES_CORE_CORE_HPP_INCLUDED

#include <type_traits>
#include <concepts>
#include <stdexcept>
#include <optional>
#include <queue>
#include <unordered_set>
#include <cstddef>
#include <memory>
#include <stack>
#include <string>

#include <cxxdes/misc/time.hpp>
#include <cxxdes/misc/utils.hpp>
#include <cxxdes/misc/reference_counted.hpp>
#include <cxxdes/misc/time.hpp>

#if (defined(__APPLE__) && defined(__clang__) && ___clang_major__ <= 13) || (defined(__clang__) && __clang_major__ <= 13)
#include <experimental/coroutine>

namespace std {
using namespace experimental;
}

#else
#include <coroutine>
#endif
namespace cxxdes {
namespace core {

namespace detail {
    template <typename Derived>
    struct await_ops_mixin;
}

struct environment;
struct coroutine_info;

using coroutine_info_ptr = memory::ptr<coroutine_info>;
using const_coroutine_info_ptr = memory::ptr<const coroutine_info>;
using coroutine_handle = std::coroutine_handle<>;

template <typename ReturnType, bool Unique>
struct coroutine;

template <typename ReturnType>
struct subroutine;

struct token_handler;
struct token;

using priority_type = std::intmax_t;
using time_integral = std::intmax_t;
using real_type = double;

using time = time_utils::time<time_integral>;
using time_utils::unitless_time;

using time_expr = time_utils::time_expr<time_integral>;
using time_units = time_utils::time_unit_type;

namespace time_ops = time_utils::ops;

constexpr auto one_second = time{1, time_units::seconds};

namespace priority_consts {

constexpr priority_type highest = std::numeric_limits<priority_type>::min();
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();
constexpr priority_type lowest = inherit - 1;
constexpr priority_type zero = static_cast<priority_type>(0);

}

struct token_handler: memory::reference_counted_base<token_handler> {
    virtual void invoke(token *) {  }
    virtual ~token_handler() {  }
};

struct token: memory::reference_counted_base<token> {
    token(time_integral time, priority_type priority, coroutine_info_ptr phandle):
        time{time},
        priority{priority},
        phandle{phandle} {  }

    // schedule time
    time_integral time = 0;

    // schedule priority
    priority_type priority = 0;

    // coroutine to continue
    coroutine_info_ptr phandle = nullptr;

    // token handler can be modified only by all and any compositions
    memory::ptr<token_handler> handler = nullptr;
};

struct no_return_value_tag {  };

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type inherited_priority,
    coroutine_info_ptr phandle) {
    { t.await_bind(env, inherited_priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(phandle) };
    { t.await_token() } -> std::same_as<token *>;
    { t.await_resume() };
    { t.await_resume(no_return_value_tag {}) } -> std::same_as<void>;
};

template <typename T>
struct immediately_return {
    T return_value;

    bool await_ready() const noexcept { return true; }
    void await_suspend(coroutine_handle) const noexcept {  }
    T await_resume() const { return return_value; }
};

// Clang needs a deduction guide
template <typename A>
immediately_return(A &&a) -> immediately_return<std::remove_cvref_t<A>>;

struct interrupted_exception: std::exception {
    interrupted_exception(std::string what = "interrupted."):
        what_{std::move(what)} {
    }

    const char *what() const noexcept override {
        return what_.c_str();
    }

private:
    std::string what_;
};

struct stopped_exception: std::exception {
    const char *what() const noexcept override {
        return "stopped.";
    }
};

struct exception_container {
    template <typename T>
    void assign(T &&t) {
        impl_.reset(new impl<std::remove_cvref_t<T>>(std::forward<T>(t)));
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return impl_.operator bool();
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    void raise() {
        impl_->raise();
    }

private:
    struct underlying_type {
        virtual void raise() = 0;
        virtual ~underlying_type() = default;
    };

    template <typename T>
    struct impl: underlying_type {
        T t;

        template <typename U>
        impl(U &&u): t{std::forward<U>(u)} {
        }

        void raise() override {
            throw t;
        }

        virtual ~impl() = default;
    };

    std::unique_ptr<underlying_type> impl_ = nullptr;
};

struct coroutine_info: memory::reference_counted_base<coroutine_info> {
    coroutine_info(util::source_location created):
        created_{created} {
    }

    coroutine_info(const coroutine_info &) = delete;
    coroutine_info &operator=(const coroutine_info &) = delete;

    coroutine_info(coroutine_info &&) = delete;
    coroutine_info &operator=(coroutine_info &&) = delete;

    util::source_location const &loc_created() const noexcept {
        return created_;
    }

    util::source_location const &loc_awaited() const noexcept {
        return awaited_;
    }

    void resume() {
        if (complete_)
            // a token might try to resume an interrupted coroutine
            return ;

        // note that coroutine<> does not pop the first coroutine
        do {
            should_continue_ = false;
            auto top = call_stack_.top();
            top.resume();
        } while (should_continue_);
    }

    [[nodiscard]]
    environment *env() noexcept {
        return env_;
    }

    [[nodiscard]]
    environment const *env() const noexcept {
        return env_;
    }

    [[nodiscard]]
    coroutine_info_ptr parent() noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    const_coroutine_info_ptr parent() const noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    priority_type priority() const noexcept {
        return priority_;
    }

    void priority(priority_type priority) noexcept {
        priority_ = priority;
    }

    [[nodiscard]]
    time_integral latency() const noexcept {
        return latency_;
    }

    void latency(time_integral latency) noexcept {
        latency_ = latency;
    }

    [[nodiscard]]
    bool started() const noexcept {
        return env_ != nullptr;
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return complete_;
    }

    template <typename T = interrupted_exception>
    void interrupt(T &&t = interrupted_exception{}) noexcept {
        exception_.assign(std::forward<T>(t));
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return exception_.valid();
    }

    virtual ~coroutine_info() = default;

protected:
    template <awaitable A>
    friend struct awaitable_wrapper;

    template <typename ReturnType, bool Unique>
    friend struct coroutine;

    template <typename ReturnType>
    friend struct subroutine;

    void bind_(environment *env, priority_type priority);
    void manage_();
    void unmanage_();

    void raise_interrupt_() {
        // clear exception_, the exception might be caught
        // we may need to interrupt again later
        auto exception = std::move(exception_);
        exception.raise();
    }

    void push_coro_(coroutine_handle coro) {
        // whenever there is a subroutine call, the
        // execution should keep moving further
        call_stack_.push(coro);
        should_continue_ = true;
    }

    void pop_coro_() {
        // whenever a subroutine call returns, the
        // execution should keep moving further
        call_stack_.pop();
        should_continue_ = true;
    }

    void destroy_if_not_started_() {
        if (ref_count() == 2 && !started()) {
            // if called, only two owners: (1) coroutine<T> and (2) promise_type
            // as the coroutine is not started yet, promise_type will never
            // get destroyed, resulting in a memory leak
            call_stack_.top().destroy();
        }
    }

protected:
    environment *env_ = nullptr;
    std::stack<coroutine_handle> call_stack_;
    bool should_continue_ = false;
    util::source_location created_;
    util::source_location awaited_;
    priority_type priority_ = priority_consts::inherit;
    time_integral latency_ = 0;
    memory::ptr<coroutine_info> parent_;
    bool complete_ = false;
    exception_container exception_;
};

struct environment {
    environment(time const &unit = one_second, time const &prec = one_second):
        now_{(time_integral) 0}, unit_{unit}, prec_{prec} {
    }

    time_integral now() const noexcept {
        return now_;
    }

    time t() const noexcept {
        return { now() * prec_.t, prec_.u };
    }

    real_type now_seconds() const noexcept {
        return t().seconds<real_type>();
    }

    void time_unit(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_unit(time x) on a used environment!");
        
        unit_ = x;
    }

    time time_unit() const noexcept {
        return unit_;
    }

    void time_precision(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_precision(time x) on a used environment!");
        
        prec_ = x;
    }

    time time_precision() const noexcept {
        return prec_;
    }

    template <cxxdes::time_utils::node Node>
    time_integral real_to_sim(Node const &n) const noexcept {
        return n.count(time_precision(), time_unit());
    }

    template <cxxdes::time_utils::scalar Scalar>
    time_integral real_to_sim(Scalar const &s) const noexcept {
        return unitless_time<Scalar>{s}.count(time_precision(), time_unit());
    }

    template <typename T>
    auto timeout(T &&t) const noexcept;

    void schedule_token(token *tkn) {
        used_ = true;
        tkn->ref();
        tokens_.push(tkn);
    }

    bool step() {
        if (tokens_.empty())
            return false;
        
        auto tkn = tokens_.top();
        tokens_.pop();

        now_ = std::max(tkn->time, now_);

        if (tkn->handler) {
            tkn->handler->invoke(tkn);
        }
        else if (tkn->phandle) {
            current_coroutine_ = tkn->phandle;
            tkn->phandle->resume();
            current_coroutine_ = nullptr;
        }

        tkn->unref();

        return true;
    }

    coroutine_info_ptr current_coroutine() const noexcept {
        return current_coroutine_;
    }

    [[nodiscard]]
    util::source_location const &loc() const noexcept {
        return loc_;
    }

    ~environment() {
        // it is not safe to iterate over the coroutinees while
        // individual coroutines might actively modify the
        // unoredered_set. move from it.
        // for a proper way to erase while iterating:
        //   https://en.cppreference.com/w/cpp/container/unordered_set/erase
        // sadly, we cannot apply this solution.

        auto coroutinees = std::move(coroutinees_);
        for (auto coroutine: coroutinees) {
            if (!coroutine->complete()) {
                coroutine->interrupt(stopped_exception{});
                coroutine->resume();
            }
        }

        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            tkn->unref();
        }
    }

private:
    time_integral now_ = 0;
    bool used_ = false;

    time unit_;
    time prec_;

    struct token_comp {
        bool operator()(token *tkn_a, token *tkn_b) const {
            return (tkn_a->time > tkn_b->time) ||
                (tkn_a->time == tkn_b->time && tkn_a->priority > tkn_b->priority);
        }
    };

    std::priority_queue<token *, std::vector<token *>, token_comp> tokens_;
    
    friend struct coroutine_info;

    template <typename ReturnType, bool Unique>
    friend struct coroutine;

    template <typename ReturnType>
    friend struct subroutine;

    template <typename Derived>
    friend struct detail::await_ops_mixin;

    std::unordered_set<memory::ptr<coroutine_info>> coroutinees_;
    coroutine_info_ptr current_coroutine_ = nullptr;
    util::source_location loc_;
};

inline
void coroutine_info::bind_(environment *env, priority_type priority) {
    if (env_) {
        if (env_ != env)
            throw std::runtime_error("cannot bind an already bound coroutine to another environment.");
        
        // already started
        return ;
    }
    
    env_ = env;
    awaited_ = env_->loc();
    parent_ = env_->current_coroutine();

    if (priority_ == priority_consts::inherit)
        priority_ = priority;
    
    auto start_token = new token{
        env_->now() + latency_,
        priority_,
        this
    };

    env_->schedule_token(start_token);
    manage_();
}

inline
void coroutine_info::manage_() {
    env_->coroutinees_.insert(this);
}

inline
void coroutine_info::unmanage_() {
    env_->coroutinees_.erase(this);
}

namespace detail {

template <typename ReturnType = void>
struct coroutine_info_return_value_mixin {
    using return_type = ReturnType;

    template <typename ...Args>
    void emplace_return_value(Args && ...args) {
        return_value_.emplace(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    bool has_return_value() const noexcept {
        return return_value_.has_value();
    }
    
    [[nodiscard]]
    ReturnType &return_value() noexcept {
        return *return_value_;
    }

    [[nodiscard]]
    ReturnType const &return_value() const noexcept {
        return *return_value_;
    }
protected:
    std::optional<ReturnType> return_value_;
};

template <>
struct coroutine_info_return_value_mixin<void> {
    using return_type = void;
};

template <bool Unique = false>
struct coroutine_info_completion_tokens_mixin {
    explicit coroutine_info_completion_tokens_mixin() {
        completion_tokens_.reserve(2);
    }

    void completion_token(token *completion_token) {
        completion_tokens_.push_back(completion_token);
    }

protected:
    void schedule_completion_(environment *env) {
        for (auto completion_token: completion_tokens_) {
            completion_token->time += env->now();
            env->schedule_token(completion_token.get());
        }
        completion_tokens_.clear();
    }

    std::vector<memory::ptr<token>> completion_tokens_;
};

template <>
struct coroutine_info_completion_tokens_mixin<true> {
    void completion_token(token *completion_token) {
        completion_token_ = completion_token;
    }

protected:
    void schedule_completion_(environment *env) {
        completion_token_->time += env->now();
        env->schedule_token(completion_token_.get());
        completion_token_ = nullptr;
    }

    memory::ptr<token> completion_token_ = nullptr;
};

template <typename Derived, typename ReturnValue>
struct coroutine_return_value_mixin {
    [[nodiscard]]
    ReturnValue const &return_value() const noexcept {
        return static_cast<Derived const *>(this)->cinfo()->return_value();
    }
};

template <typename Derived>
struct coroutine_return_value_mixin<Derived, void> {
};

} /* namespace detail */


template <typename ReturnType = void, bool Unique = false>
struct coroutine_info_:
    coroutine_info,
    detail::coroutine_info_completion_tokens_mixin<Unique>,
    detail::coroutine_info_return_value_mixin<ReturnType> {
    using coroutine_info::coroutine_info;

    void do_return() {
        this->schedule_completion_(env_); // this-> is a must here
        this->complete_ = true;
        this->unmanage_();
    }

    virtual ~coroutine_info_() = default;
};

template <awaitable A>
struct awaitable_wrapper {
    A a;
    coroutine_info_ptr phandle_this = nullptr;
    coroutine_info_ptr phandle_old = nullptr;
    
    bool await_ready() {
        return a.await_ready();
    }
    
    void await_suspend(coroutine_handle) {
        a.await_suspend(phandle_old);
    }

    auto await_resume() {
        if (phandle_this->interrupted())
            phandle_this->raise_interrupt_();
        return a.await_resume();
    }
};

struct this_coroutine {  };
struct this_environment {  };

template <typename T>
struct await_transform_extender;

namespace detail {

template <typename Derived>
struct await_ops_mixin {
    template <awaitable A>
    auto await_transform(
        A &&a,
        util::source_location const loc = util::source_location::current()) {
        auto result = awaitable_wrapper<std::remove_cvref_t<A>>{
            std::forward<A>(a),
            derived().cinfo.get(),
            derived().cinfo->env()->current_coroutine()
        };
        derived().cinfo->env()->loc_ = loc;
        result.a.await_bind(
            derived().cinfo->env(),
            derived().cinfo->priority());
        return result;
    }

    template <typename ReturnType>
    auto &&await_transform(subroutine<ReturnType> &&a);

    template <typename T>
    auto await_transform(
        await_transform_extender<T> const &a,
        util::source_location const loc = util::source_location::current()) {
        return a.await_transform(derived().cinfo, loc);
    }

    auto await_transform(this_coroutine) noexcept {
        return immediately_return{derived().cinfo};
    }

    auto await_transform(this_environment) noexcept {
        return immediately_return{derived().cinfo->env()};
    }
    
    template <awaitable A>
    auto yield_value(A &&a) {
        return await_transform(std::forward<A>(a));
    }

private:
    Derived &derived() noexcept {
        return *static_cast<Derived *>(this);
    }

    Derived const &derived() const noexcept {
        return *static_cast<Derived const *>(this);
    }
};

} /* namespace detail */

template <typename ReturnType = void>
struct subroutine {
    static_assert(
        not std::is_same_v<ReturnType, void>,
        "void-returning subroutines not implemented yet");

    struct promise_type;

    subroutine() noexcept: h_{nullptr} {  }

    subroutine(subroutine const &) noexcept = delete;
    subroutine &operator=(subroutine const &other) noexcept = delete;

    subroutine(subroutine &&other) noexcept {
        *this = std::move(other);
    }

    subroutine &operator=(subroutine &&other) noexcept {
        if (this != &other) std::swap(h_, other.h_);
        return *this;
    }

    bool await_ready() {
        if (h_) return h_.done();
        return true;
    }

    void await_suspend(std::coroutine_handle<>) {
        auto &promise = h_.promise();
        promise.cinfo->push_coro_(h_);
    }

    ReturnType await_resume() {
        auto &promise = h_.promise();
        if (promise.eptr)
            std::rethrow_exception(promise.eptr);
        
        return std::move(*promise.ret);
    }

    ~subroutine() {
        if (h_) h_.destroy();
    }

private:
    template <typename>
    friend struct detail::await_ops_mixin;

    void bind_coroutine_(coroutine_info_ptr cinfo) {
        auto &promise = h_.promise();
        promise.cinfo = std::move(cinfo);
    }

public:
    struct promise_type: detail::await_ops_mixin<promise_type> {
        std::coroutine_handle<promise_type> h = nullptr;
        std::exception_ptr eptr = nullptr;
        std::optional<ReturnType> ret;
        coroutine_info_ptr cinfo = nullptr;

        promise_type() {
            h = std::coroutine_handle<promise_type>::from_promise(*this);
        }

        subroutine get_return_object() noexcept {
            return subroutine(h);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept {
            struct final_awaitable {
                coroutine_info_ptr cinfo;

                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<>) noexcept {
                    cinfo->pop_coro_();
                }
                void await_resume() noexcept {  }
            };
            return final_awaitable{cinfo};
        }

        auto unhandled_exception() {
            eptr = std::current_exception();
        }

        template <typename T>
        void return_value(T &&t) {
            ret.emplace(std::forward<T>(t));
        }

        template <typename>
        friend struct subroutine;
    };

private:
    std::coroutine_handle<promise_type> h_ = nullptr;

    explicit
    subroutine(std::coroutine_handle<promise_type> h) noexcept: h_{h} {
    }
};

namespace detail {

template <typename Derived>
template <typename ReturnType>
auto &&await_ops_mixin<Derived>::await_transform(subroutine<ReturnType> &&a) {
    a.bind_coroutine_(derived().cinfo.get());
    return std::move(a);
}

} /* namespace detail */

template <typename ReturnType = void, bool Unique = false>
struct coroutine:
    detail::coroutine_return_value_mixin<coroutine<ReturnType, Unique>, ReturnType> {
    using coroutine_info_type = coroutine_info_<ReturnType, Unique>;

    explicit
    coroutine(memory::ptr<coroutine_info_type> cinfo = nullptr):
        cinfo_{std::move(cinfo)} {
    }

    coroutine(coroutine const &other) requires (not Unique) {
        *this = other;
    }

    coroutine &operator=(coroutine const &other) requires (not Unique) {
        if (this != &other) {
            cinfo_ = other.cinfo_;
            completion_token_ = nullptr;
            return_ = other.return_;
        }
        return *this;
    }

    coroutine(coroutine const &) requires (Unique) = delete;
    coroutine &operator=(coroutine const &) requires (Unique) = delete;

    coroutine(coroutine &&other) {
        *this = std::move(other);
    }

    coroutine &operator=(coroutine &&other) {
        if (this != &other) {
            cinfo_ = std::move(other.cinfo_);
            std::swap(completion_token_, other.completion_token_);
            std::swap(return_, return_);
        }
        return *this;
    }

    ~coroutine() {
        if (cinfo_) cinfo_->destroy_if_not_started_();
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return cinfo_;
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    coroutine_info_type const *cinfo() const noexcept {
        return cinfo_.get();
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return cinfo_->complete();
    }

    template <typename T = interrupted_exception>
    void interrupt(T &&t = interrupted_exception{}) noexcept {
        cinfo_->interrupt(std::forward<T>(t));
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return cinfo_->interrupted();
    }

    [[nodiscard]]
    priority_type priority() const noexcept {
        return cinfo_->priority();
    }

    auto &priority(priority_type priority) noexcept {
        cinfo_->priority(priority);
        return *this;
    }

    [[nodiscard]]
    time_integral latency() const noexcept {
        return cinfo_->latency();
    }

    auto &latency(time_integral latency) noexcept {
        cinfo_->latency(latency);
        return *this;
    }

    [[nodiscard]]
    priority_type return_priority() const noexcept {
        return return_.priority;
    }

    auto &return_priority(priority_type priority) noexcept {
        return_.priority = priority;
        return *this;
    }

    [[nodiscard]]
    time_integral return_latency() const noexcept {
        return return_.latency;
    }

    auto &return_latency(time_integral latency) noexcept {
        return_.latency = latency;
        return *this;
    }

    void await_bind(environment *env, priority_type priority = 0) {
        cinfo_->bind_(env, priority);
    }

    bool await_ready() const noexcept {
        return cinfo_->complete();
    }

    void await_suspend(coroutine_info_ptr phandle) {
        if (completion_token_)
            throw std::runtime_error("coroutine<> is already being awaited!");

        completion_token_ = new token{return_.latency, return_.priority, phandle};
        if (completion_token_->priority == priority_consts::inherit)
            completion_token_->priority = cinfo_->priority_;
        cinfo_->completion_token(completion_token_);
    }

    token *await_token() const noexcept {
        return completion_token_;
    }

    ReturnType await_resume() requires (not Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!cinfo_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return cinfo_->return_value();
        }
    }

    ReturnType await_resume() requires (Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!cinfo_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return std::move(cinfo_->return_value());
        }
    }

    void await_resume(no_return_value_tag) noexcept {
        completion_token_ = nullptr;
    }

private:
    memory::ptr<coroutine_info_type> cinfo_ = nullptr;
    token *completion_token_ = nullptr; // must be non-owning, in case of copies

    struct {
        time_integral latency = 0;
        priority_type priority = priority_consts::inherit;
    } return_;

    template <typename Derived>
    struct return_value_mixin {
        template <typename T>
        void return_value(T &&t) {
            auto &derived = *static_cast<Derived *>(this);
            derived.emplace_return_value(std::forward<T>(t));
            derived.do_return();
        }
    };

    template <typename Derived>
    struct return_void_mixin {
        void return_void() {
            auto &derived = *static_cast<Derived *>(this);
            derived.do_return();
        }
    };

public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        >, detail::await_ops_mixin<promise_type> {
        memory::ptr<coroutine_info_type> cinfo;
    
        template <typename ...Args>
        promise_type(Args && ...args) {
            auto loc = util::extract_first_type<util::source_location>(args...);
            auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
            cinfo = new coroutine_info_type(loc);
            cinfo->push_coro_(coro);
        }

        coroutine get_return_object() {
            return coroutine(cinfo);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }

        auto unhandled_exception() -> void {
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (stopped_exception & /* ex */) {
                do_return();
            }
            catch (...) {
                std::rethrow_exception(std::current_exception());
            }
        }

        template <typename T>
        void emplace_return_value(T &&t) {
            cinfo->emplace_return_value(std::forward<T>(t));
        }

        void do_return() {
            cinfo->do_return();
        }

        ~promise_type() {
        }
    };
};

template <typename ReturnValue = void>
using unique_coroutine = coroutine<ReturnValue, true>;

template <typename T>
concept releasable = requires(T t) {
    { t.release() } -> awaitable;
};

template <typename T>
concept acquirable = requires(T t) {
    { t.acquire() } -> awaitable;
    { t.acquire().await_resume() } -> releasable;
};

namespace detail {

struct under_helper {  };

} /* namespace detail */

} /* namespace core */
} /* namespace cxxdes */

template <cxxdes::core::acquirable A, typename F>
cxxdes::core::coroutine<void> operator+(A &a, F &&f) {
    auto handle = co_await a.acquire();
    co_await std::forward<F>(f)();
    co_await handle.release();
}

#define co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::coroutine<void>

template <typename F>
auto operator+(cxxdes::core::detail::under_helper, F f) {
    return f();
}

#define _coroutine(...) cxxdes::core::detail::under_helper{} + [&](__VA_ARGS__) -> coroutine<void>

#endif /* CXXDES_CORE_CORE_HPP_INCLUDED */
