#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

struct example {
    constexpr example(time_integral latency, priority_type priority = priority_consts::inherit):
        latency_{latency}, priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        // inherit the priority if necessary
        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(coroutine_data_ptr coro_data) {
        tkn_ = new token(env_->now() + latency_, priority_, coro_data, "example awaitable");

        // create a token and do something with it
        env_->schedule_token(tkn_); // like, scheduling it
    }

    token *await_token() const noexcept {
        return tkn_;
    }

    int await_resume() const noexcept {
        // return something here
        return 4;
    }

    void await_resume(no_return_value_tag) const noexcept {  }
private:
    environment *env_ = nullptr;
    token *tkn_ = nullptr;
    time_integral latency_;
    priority_type priority_;
};

int main() { return 0; }
