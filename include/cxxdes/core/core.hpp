/**
 * @file core.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief process and environment.
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

#include <cxxdes/core/defs.hpp>
#include <cxxdes/core/coroutine.hpp>
#include <cxxdes/misc/utils.hpp>
#include <cxxdes/misc/reference_counted.hpp>
#include <cxxdes/misc/time.hpp>

namespace cxxdes {
namespace core {

struct environment;

struct basic_process_data;
using process_handle = basic_process_data *;
using const_process_handle = basic_process_data const *;

template <typename ReturnType, bool Unique>
struct process;

struct token_handler;
struct token;

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
    token(time_integral time, priority_type priority, process_handle phandle):
        time{time},
        priority{priority},
        phandle{phandle} {  }

    // schedule time
    time_integral time = 0;

    // schedule priority
    priority_type priority = 0;

    // coroutine to continue
    process_handle phandle = nullptr;

    // token handler can be modified only by all and any compositions
    memory::ptr<token_handler> handler = nullptr;
};

struct no_return_value_tag {  };

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type inherited_priority,
    process_handle phandle) {
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
    void await_suspend(coro_handle) const noexcept {  }
    T await_resume() const { return return_value; }
};

// Clang needs a deduction guide
template <typename A>
immediately_return(A &&a) -> immediately_return<std::remove_cvref_t<A>>;

struct basic_process_data: memory::reference_counted_base<basic_process_data> {
    basic_process_data(coro_handle coro, util::source_location created):
        coro_{coro}, created_{created} {
    }

    basic_process_data(const basic_process_data &) = delete;
    basic_process_data &operator=(const basic_process_data &) = delete;

    basic_process_data(basic_process_data &&) = delete;
    basic_process_data &operator=(basic_process_data &&) = delete;

    util::source_location const &loc_created() const noexcept {
        return created_;
    }

    util::source_location const &loc_awaited() const noexcept {
        return awaited_;
    }

    void resume() {
        if (coro_)
            return coro_.resume();
    }

    [[nodiscard]]
    coro_handle coro() const noexcept {
        return coro_;
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
    process_handle parent() noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    const_process_handle parent() const noexcept {
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

    void interrupt() noexcept {
        interrupt_ = true;
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return interrupt_;
    }

    virtual ~basic_process_data() = default;

protected:
    template <typename ReturnType, bool Unique>
    friend struct process;

    void bind_(environment *env, priority_type priority);
    void manage_();
    void unmanage_();

protected:
    environment *env_ = nullptr;
    coro_handle coro_ = nullptr;
    util::source_location created_;
    util::source_location awaited_;
    priority_type priority_ = priority_consts::inherit;
    time_integral latency_ = 0;
    memory::ptr<basic_process_data> parent_;
    bool complete_ = false;
    bool interrupt_ = false;
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
            current_process_ = tkn->phandle;
            tkn->phandle->resume();
            current_process_ = nullptr;
        }

        tkn->unref();

        return true;
    }

    process_handle current_process() const noexcept {
        return current_process_;
    }

    [[nodiscard]]
    util::source_location const &loc() const noexcept {
        return loc_;
    }

    void loc(util::source_location const &loc) noexcept {
        loc_ = loc;
    }

    ~environment() {
        for (auto process: processes_) {
            if (!process->complete()) {
                process->interrupt();
                process->resume();
            }
        }
        processes_.clear();

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
    
    friend struct basic_process_data;

    std::unordered_set<memory::ptr<basic_process_data>> processes_;
    process_handle current_process_ = nullptr;
    util::source_location loc_;
};

inline
void basic_process_data::bind_(environment *env, priority_type priority) {
    if (env_) {
        if (env_ != env)
            throw std::runtime_error("cannot bind an already bound process to another environment.");
        
        // already started
        return ;
    }
    
    env_ = env;
    awaited_ = env_->loc();
    parent_ = env_->current_process();

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
void basic_process_data::manage_() {
    env_->processes_.insert(this);
}

inline
void basic_process_data::unmanage_() {
    env_->processes_.erase(this);
}

namespace detail {

template <typename ReturnType = void>
struct process_data_return_value_mixin {
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
struct process_data_return_value_mixin<void> {
};

template <bool Unique = false>
struct process_data_completion_tokens_mixin {
    explicit process_data_completion_tokens_mixin() {
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
struct process_data_completion_tokens_mixin<true> {
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

} /* namespace detail */


template <typename ReturnType = void, bool Unique = false>
struct process_data:
    basic_process_data,
    detail::process_data_completion_tokens_mixin<Unique>,
    detail::process_data_return_value_mixin<ReturnType> {
    using basic_process_data::basic_process_data;

    void do_return() {
        this->schedule_completion_(env_); // this-> is a must here
        this->complete_ = true;
        this->unmanage_();
    }

    virtual ~process_data() = default;
};

template <awaitable A, typename Exception>
struct awaitable_wrapper {
    A &a;
    process_handle phandle_this = nullptr;
    process_handle phandle_old = nullptr;
    
    bool await_ready() {
        return a.await_ready();
    }
    
    void await_suspend(coro_handle) {
        a.await_suspend(phandle_old);
    }

    auto await_resume() {
        if (phandle_this->interrupted())
            throw Exception{};
        return a.await_resume();
    }
};

struct this_process {  };
struct this_environment {  };

template <typename T>
struct await_transform_extender;

template <typename ReturnType = void, bool Unique = false>
struct process {
    using process_data_type = process_data<ReturnType, Unique>;

    explicit
    process(memory::ptr<process_data_type> const &pdata = nullptr):
        pdata_{pdata} {
    }

    process(process const &) requires (not Unique) = default;
    process &operator=(process const &) requires (not Unique) = default;

    process(process const &) requires (Unique) = delete;
    process &operator=(process const &) requires (Unique) = delete;

    process(process &&) = default;
    process &operator=(process &&) = default;

    ~process() {
        if (pdata_ && pdata_->ref_count() == 2 && !pdata_->started()) {
            pdata_->coro().destroy();
        }
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return pdata_;
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return pdata_;
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return pdata_->complete();
    }

    void interrupt() noexcept {
        pdata_->interrupt();
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return pdata_->interrupted();
    }

    priority_type priority() const noexcept {
        return pdata_->priority();
    }

    auto &priority(priority_type priority) noexcept {
        pdata_->priority(priority);
        return *this;
    }

    time_integral latency() const noexcept {
        return pdata_->latency();
    }

    auto &latency(time_integral latency) noexcept {
        pdata_->latency(latency);
        return *this;
    }

    priority_type return_priority() const noexcept {
        return return_.priority;
    }

    auto &return_priority(priority_type priority) noexcept {
        return_.priority = priority;
        return *this;
    }

    time_integral return_latency() const noexcept {
        return return_.latency;
    }

    auto &return_latency(time_integral latency) noexcept {
        return_.latency = latency;
        return *this;
    }

    void await_bind(environment *env, priority_type priority = 0) {
        pdata_->bind_(env, priority);
    }

    bool await_ready() const noexcept {
        return pdata_->complete();
    }

    void await_suspend(process_handle phandle) {
        if (completion_token_)
            throw std::runtime_error("process<> is already being awaited");

        completion_token_ = new token{return_.latency, return_.priority, phandle};
        if (completion_token_->priority == priority_consts::inherit)
            completion_token_->priority = pdata_->priority_;
        pdata_->completion_token(completion_token_);
    }

    token *await_token() const noexcept {
        return completion_token_;
    }

    ReturnType await_resume() requires (not Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!pdata_->has_return_value())
                throw std::runtime_error("no return value from the process<T> [T != void]!");

            return pdata_->return_value();
        }
    }

    ReturnType await_resume() requires (Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!pdata_->has_return_value())
                throw std::runtime_error("no return value from the process<T> [T != void]!");

            return std::move(pdata_->return_value());
        }
    }

    void await_resume(no_return_value_tag) noexcept {
        completion_token_ = nullptr;
    }

private:
    memory::ptr<process_data_type> pdata_ = nullptr;
    token *completion_token_ = nullptr; // must be non-owning, in case of copies

    struct {
        time_integral latency = 0;
        priority_type priority = priority_consts::inherit;
    } return_;

    template <typename Derived>
    struct return_value_mixin {
        template <typename T>
        void return_value(T &&t) {
            static_cast<Derived *>(this)->emplace_return_value(std::forward<T>(t));
            static_cast<Derived *>(this)->do_return();
        }
    };

    template <typename Derived>
    struct return_void_mixin {
        void return_void() {
            static_cast<Derived *>(this)->do_return();
        }
    };

public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        > {
    
        template <typename ...Args>
        promise_type(Args && ...args) {
            auto loc = util::extract_first_type<util::source_location>(args...);
            auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
            pdata_ = new process_data_type(coro, loc);
        }

        process get_return_object() {
            return process(pdata_);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }
        auto unhandled_exception() -> void {
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (interrupted_exception & /* ex */) {
            }
            catch (...) {
                std::rethrow_exception(std::current_exception());
            }
        }

        template <awaitable A>
        auto await_transform(
            A &&a,
            util::source_location const loc = util::source_location::current()) {
            // co_await (A{});
            // A{} is alive throughout the co_await expression
            // therefore, it is safe to return keep a reference to it

            auto result = awaitable_wrapper<A, interrupted_exception>{
                a,
                pdata_.get(),
                pdata_->env()->current_process()
            };
            pdata_->env()->loc(loc);
            a.await_bind(pdata_->env(), pdata_->priority());
            return result;
        }

        template <typename T>
        auto await_transform(
            await_transform_extender<T> const &a,
            util::source_location const loc = util::source_location::current()) {
            return a.await_transform(pdata_.get(), loc);
        }

        auto await_transform(this_process) noexcept {
            return immediately_return{pdata_.get()};
        }

        auto await_transform(this_environment) noexcept {
            return immediately_return{pdata_->env()};
        }
        
        template <awaitable A>
        auto yield_value(A &&a) {
            return await_transform(std::forward<A>(a));
        }

        template <typename T>
        void emplace_return_value(T &&t) {
            pdata_->emplace_return_value(std::forward<T>(t));
        }

        void do_return() {
            pdata_->do_return();
        }

        ~promise_type() {
        }
    private:
        memory::ptr<process_data_type> pdata_;

        struct interrupted_exception: std::exception {
            const char *what() const noexcept override {
                return "interrupted.";
            }
        };
    };
};

template <typename ReturnValue = void>
using unique_process = process<ReturnValue, true>;

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
cxxdes::core::process<void> operator+(A &a, F &&f) {
    auto handle = co_await a.acquire();
    co_await std::forward<F>(f)();
    co_await handle.release();
}

#define co_with(x) co_yield (x) + [&]() mutable -> cxxdes::core::process<void>

template <typename F>
auto operator+(cxxdes::core::detail::under_helper, F f) {
    return f();
}

#define _Process(...) cxxdes::core::detail::under_helper{} + [&](__VA_ARGS__) -> process<void>

#endif /* CXXDES_CORE_CORE_HPP_INCLUDED */
