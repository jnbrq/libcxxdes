#if (defined(__APPLE__) && defined(__clang__) && ___clang_major__ <= 13) || (defined(__clang__) && __clang_major__ <= 13)
#include <experimental/coroutine>

namespace std {
using namespace experimental;
}

#else
#include <coroutine>
#endif

#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <cstdint>
#include <limits>
#include <queue>
#include <optional>

using priority_type = std::intmax_t;
using time_type = std::uintmax_t;
using coro_handle = std::coroutine_handle<>;

namespace priority_consts {

constexpr priority_type highest = std::numeric_limits<priority_type>::min();
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();
constexpr priority_type lowest = inherit - 1;
constexpr priority_type zero = static_cast<priority_type>(0);

}

struct empty_type {};

struct token;

struct token_handler {
    virtual bool invoke(token *tkn) { return true; }
    virtual ~token_handler() {  }
};

struct token {
    token(time_type time, priority_type priority, coro_handle coro):
        time{time},
        priority{priority},
        coro{coro} {  }
        

    time_type time = 0;
    priority_type priority = 0;
    coro_handle coro = nullptr;

    token_handler *handler = nullptr;

    void process() {
        if (handler != nullptr)
            if (not handler->invoke(this))
                return ;
        if (coro && !coro.done())
            coro.resume();
    }

    ~token() {
        if (handler) delete handler;
    }
};

struct environment {
    environment() {  }

    time_type now() const {
        return now_;
    }

    void schedule_token(token *tkn) {
        tokens_.push(tkn);
    }

    bool step() {
        if (tokens_.empty())
            return false;
        
        auto tkn = tokens_.top();
        tokens_.pop();

        now_ = std::max(tkn->time, now_);
        tkn->process();

        delete tkn;

        return true;
    }

    ~environment() {
        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            delete tkn;
        }
    }

private:
    time_type now_ = 0;

    struct token_comp {
        bool operator()(token *tkn_a, token *tkn_b) const {
            return (tkn_a->time > tkn_b->time) ||
                (tkn_a->time == tkn_b->time && tkn_a->priority > tkn_b->priority);
        }
    };

    std::priority_queue<token *, std::vector<token *>, token_comp> tokens_;
};

template <typename ReturnType>
struct process;

template <typename T>
concept awaitable = requires(
    T t,
    environment *env,
    priority_type priority,
    coro_handle coro) {
    { t.await_bind(env, priority) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(coro) };
    { t.await_resume() };
};

template <typename T>
struct immediately_returning_awaitable {
    template <typename ...Args>
    immediately_returning_awaitable(Args &&...args): t_{std::forward<Args>(args)...} {  }

    void await_bind(environment *, priority_type) const noexcept {  }
    bool await_ready() const noexcept { return true; }
    void await_suspend(coro_handle) const noexcept {  }
    T await_resume() { return std::move(t_); }
private:
    T t_;
};

struct this_process {
    struct get_return_latency {  };

    struct get_return_priority {  };

    struct set_return_latency {
        time_type latency;
    };

    struct set_return_priority {
        priority_type priority;
    };
    
    struct get_priority {  };

    struct set_priority {
        priority_type priority;
    };

    struct get_environment {  };
};

template <typename ReturnType = void>
struct process {
    struct promise_type;

    process(promise_type *this_promise): this_promise_{this_promise} {
    }

    void await_bind(environment *env, priority_type priority = priority_consts::zero) {
        this_promise_->bind(env);
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro_handle current_coro) noexcept {
        completion_tkn_ = new token{this_promise_->env->now(), this_promise_->priority, current_coro};
        if constexpr (not std::is_same_v<ReturnType, void>)
            completion_tkn_->handler = new return_value_handler{};
        this_promise_->completion_tkn = completion_tkn_;
    }

    ReturnType await_resume() {
        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            // at this point, the promise is already destroyed,
            // however, the completion token is still alive because tokens are deleted after coro is resumed.
            auto &return_value = ((return_value_handler *) completion_tkn_->handler)->return_value;
            if (!return_value)
                throw std::runtime_error("no return value from the process<T> [T != void]!");
            return std::move(*return_value);
        }
    }

    auto &priority(priority_type priority) {
        #ifdef CXXDES_SAFE
        if (!this_promise_->start_tkn)
            throw std::runtime_error("cannot change the priority of a started process");
        #endif
        this_promise_->start_tkn->priority = priority;
        return *this;
    }

    auto &latency(time_type latency) {
        #ifdef CXXDES_SAFE
        if (!this_promise_->start_tkn)
            throw std::runtime_error("cannot change the latency of a started process");
        #endif
        this_promise_->start_tkn->time = latency;
        return *this;
    }

private:
    // we need these mixins, because return_value and return_void cannot coexist.
    // even with concepts, it does not work.

    template <typename Derived>
    struct return_value_mixin {
        template <typename T>
        void return_value(T &&t) {
            ((Derived &) *this).set_result(std::forward<T>(t));
            ((Derived &) *this).do_return();
        }
    };

    template <typename Derived>
    struct return_void_mixin {
        void return_void() {
            ((Derived &) *this).do_return();
        }
    };

    struct return_value_handler: token_handler {
        std::optional<ReturnType> return_value;
        virtual ~return_value_handler() {  }
    };
    
public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        > {
        
        // environment that this process is bound to
        environment *env = nullptr;

        // start token
        token *start_tkn = nullptr;

        // completion token
        token *completion_tkn = nullptr;

        // correspoding coroutine object
        coro_handle this_coro = nullptr;

        // priority to be inherited by the subsequent co_await's
        priority_type priority = 0;

        template <typename ...Args>
        promise_type(Args && ...) {
            start_tkn = new token{0, priority_consts::zero, nullptr};
            this_coro = std::coroutine_handle<promise_type>::from_promise(*this);
        };

        process get_return_object() {
            return process(this);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }
        auto unhandled_exception() { std::rethrow_exception(std::current_exception()); }

        template <typename A>
        A &&await_transform(A &&a) const noexcept {
            // co_await (A{});
            // A{} is alive throughout the co_await expression
            // therefore, it is safe to return an rvalue-reference to it

            a.await_bind(env, priority);
            return std::move(a);
        }

        // implementation of the this_process interface

        auto await_transform(this_process::get_return_latency) const {
            if (!completion_tkn) {
                throw std::runtime_error("get_return_latency cannot be called for the main process!");
            }

            return immediately_returning_awaitable<time_type>{completion_tkn->time};
        }

        auto await_transform(this_process::set_return_latency x) const {
            if (!completion_tkn) {
                throw std::runtime_error("set_return_latency cannot be called for the main process!");
            }

            completion_tkn->time = x.latency;
            return std::suspend_never{};
        }
        
        auto await_transform(this_process::get_return_priority) const {
            if (!completion_tkn) {
                throw std::runtime_error("get_return_priority cannot be called for the main process!");
            }

            return immediately_returning_awaitable<priority_type>{completion_tkn->priority};
        }

        auto await_transform(this_process::set_return_priority x) const {
            if (!completion_tkn) {
                throw std::runtime_error("set_return_priority cannot be called for the main process!");
            }

            completion_tkn->priority = x.priority;
            return std::suspend_never{};
        }

        auto await_transform(this_process::get_priority) const {
            return immediately_returning_awaitable<priority_type>{priority};
        }

        auto await_transform(this_process::set_priority x) const {
            priority = x.priority;
            return std::suspend_never{};
        }

        auto await_transform(this_process::get_environment) const {
            return immediately_returning_awaitable<environment *>{env};
        }

        void bind(environment *env) {
            if (start_tkn) {
                this->env = env;
                start_tkn->coro = this_coro;
                env->schedule_token(start_tkn);
                priority = start_tkn->priority;
                start_tkn = nullptr;
            }
        }
        
        template <typename T>
        void set_result(T &&t) {
            if (completion_tkn) {
                ((return_value_handler *) completion_tkn->handler)->return_value = std::forward<T>(t);
            }
        }

        void do_return() {
            if (completion_tkn) {
                completion_tkn->time += env->now();
                env->schedule_token(completion_tkn);
                completion_tkn = nullptr;
            }
        }

        ~promise_type() {
            if (start_tkn) delete start_tkn;
        }
    };

private:
    promise_type *this_promise_ = nullptr;
    token *completion_tkn_ = nullptr;
};

struct timeout {
    constexpr timeout(time_type latency, priority_type priority = priority_consts::inherit):
        latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const {
        return false;
    }

    void await_suspend(coro_handle current_coro) const {
        auto tkn = new token(env_->now() + latency_, priority_, current_coro);
        env_->schedule_token(tkn);
    }

    void await_resume() {
    }
private:
    environment *env_ = nullptr;
    time_type latency_ = 0;
    priority_type priority_ = 1000;
};



#include <fmt/core.h>

process<int> g(int k = 10) {
    if (k == 0)
        co_return 0;
    co_return k + co_await g(k - 1);
}

process<int> f() {
    co_return ((co_await g()) + 10);
}

process<void> test() {
    auto this_env = co_await this_process::get_environment();
    co_await timeout(10);
    fmt::print("from {}, now = {}\n", __PRETTY_FUNCTION__, this_env->now());
    auto result = co_await f();
    fmt::print("from {}, now = {} and result = {}\n", __PRETTY_FUNCTION__, this_env->now(), result);
    auto priority = co_await this_process::get_priority();
    fmt::print("from {}, now = {} and priority = {}\n", __PRETTY_FUNCTION__, this_env->now(), priority);
}

int main() {
    environment env;

    auto p = test().priority(200);
    p.await_bind(&env);

    while (env.step()) ;

    return 0;
}
