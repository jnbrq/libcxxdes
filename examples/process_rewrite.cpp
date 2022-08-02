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
#include <queue>
#include <optional>

using priority_type = std::intmax_t;
using time_type = std::uintmax_t;
using coro_handle = std::coroutine_handle<>;

struct empty_type {};

struct event;

struct event_handler {
    virtual bool invoke(event *evt) { return true; }
    virtual ~event_handler() {  }
};

struct event {
    event(time_type time, priority_type priority, coro_handle coro):
        time{time},
        priority{priority},
        coro{coro} {  }

    time_type time = 0;
    priority_type priority = 0;
    coro_handle coro = nullptr;

    event_handler *handler = nullptr;

    void process() {
        if (handler != nullptr)
            if (not handler->invoke(this))
                return ;
        if (coro && !coro.done())
            coro.resume();
    }

    ~event() {
        if (handler) delete handler;
    }
};

struct environment {
    environment() {  }

    time_type now() const {
        return now_;
    }

    void append_event(event *evt) {
        events_.push(evt);
    }

    bool step() {
        if (events_.empty())
            return false;
        
        auto evt = events_.top();
        events_.pop();

        now_ = std::max(evt->time, now_);
        evt->process();

        delete evt;

        return true;
    }

    ~environment() {
        while (!events_.empty()) {
            auto evt = events_.top();
            events_.pop();
            delete evt;
        }
    }

private:
    time_type now_ = 0;

    struct event_comp {
        bool operator()(event *evt_a, event *evt_b) const {
            return (evt_a->time > evt_b->time) ||
                (evt_a->time == evt_b->time && evt_a->priority > evt_b->priority);
        }
    };

    std::priority_queue<event *, std::vector<event *>, event_comp> events_;
};

template <typename T>
concept awaitable = requires(T t, environment *env, coro_handle coro) {
    { t.await_bind(env) };
    { t.await_ready() } -> std::same_as<bool>;
    { t.await_suspend(coro) };
    { t.await_resume() };
};

template <typename ReturnType = void>
struct process {
    struct promise_type;

    process(promise_type *this_promise): this_promise_{this_promise} {
    }

    auto &await_bind(environment *env) {
        this_promise_->bind(env);
        return *this;
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coro_handle current_coro) noexcept {
        completion_evt_ = new event{this_promise_->env->now(), 1000, current_coro};
        if constexpr (not std::is_same_v<ReturnType, void>)
            completion_evt_->handler = new return_value_handler{};
        this_promise_->completion_evt = completion_evt_;
    }

    ReturnType await_resume() {
        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            // at this point, the promise is already destroyed,
            // however, the completion event is still alive because events are deleted after coro is resumed.
            auto &return_value = ((return_value_handler *) completion_evt_->handler)->return_value;
            if (!return_value)
                throw std::runtime_error("no return value from the process<T> [T != void]!");
            return std::move(*return_value);
        }
    }

    auto &priority(priority_type priority) {
        #ifdef CXXDES_SAFE
        if (!this_promise_->start_event)
            throw std::runtime_error("cannot change the priority of a started process");
        #endif
        this_promise_->start_event->priority = priority;
        return *this;
    }

    auto &latency(time_type latency) {
        #ifdef CXXDES_SAFE
        if (!this_promise_->start_event)
            throw std::runtime_error("cannot change the latency of a started process");
        #endif
        this_promise_->start_event->time = latency;
        return *this;
    }

private:
    // we need these mixins, because return_value and return_void cannot coexist.
    // even with concepts, it does not help.

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

    struct return_value_handler: event_handler {
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
        
        environment *env = nullptr;
        event *start_evt = nullptr;
        event *completion_evt = nullptr;
        coro_handle this_coro = nullptr;

        template <typename ...Args>
        promise_type(Args && ...) {
            start_evt = new event{0, -1000, nullptr};
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
            a.await_bind(env);
            return std::move(a);
        }

        void bind(environment *env) {
            if (start_evt) {
                this->env = env;
                start_evt->coro = this_coro;
                env->append_event(start_evt);
                start_evt = nullptr;
            }
        }
        
        template <typename T>
        void set_result(T &&t) {
            if (completion_evt) {
                ((return_value_handler *) completion_evt->handler)->return_value = std::forward<T>(t);
            }
        }

        void do_return() {
            if (completion_evt) {
                completion_evt->time += env->now();
                env->append_event(completion_evt);
                completion_evt = nullptr;
            }
        }

        ~promise_type() {
            if (start_evt) delete start_evt;
        }
    };

private:
    promise_type *this_promise_ = nullptr;
    event *completion_evt_ = nullptr;
};

struct timeout {
    constexpr timeout(time_type latency, priority_type priority = 1000):
        latency_{latency}, priority_{priority} {
    }

    auto &await_bind(environment *env) {
        env_ = env;
        return *this;
    }

    bool await_ready() const {
        return false;
    }

    void await_suspend(coro_handle current_coro) const {
        auto evt = new event(env_->now() + latency_, priority_, current_coro);
        env_->append_event(evt);
    }

    void await_resume() {
    }
private:
    environment *env_ = nullptr;
    time_type latency_ = 0;
    priority_type priority_ = 1000;
};

inline auto get_env() {
    struct get_env_type {
        constexpr get_env_type() {  }

        auto &await_bind(environment *env) {
            env_ = env;
            return *this;
        }

        bool await_ready() const {
            return true;
        }

        void await_suspend(coro_handle) const {
        }

        environment *await_resume() const {
            return env_;
        }

    private:
        environment *env_ = nullptr;
    };

    return get_env_type{};
}

#include <fmt/core.h>

process<int> f() {
    co_return 10;
}

process<void> test() {
    auto this_env = co_await get_env();
    co_await timeout(10);
    fmt::print("from {}, now = {}\n", __PRETTY_FUNCTION__, this_env->now());
    auto result = co_await f();
    fmt::print("from {}, now = {} and result = {}\n", __PRETTY_FUNCTION__, this_env->now(), result);
}

int main() {
    environment env;

    auto p = test();
    p.await_bind(&env);

    while (env.step()) ;

    return 0;
}
