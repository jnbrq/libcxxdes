#include <coroutine>
#include <fmt/core.h>
#include <stack>

std::stack<std::coroutine_handle<>> call_stack;

template <typename T = int>
struct subroutine {
    struct promise_type;

    subroutine() noexcept: h_{nullptr} {  }

    subroutine(subroutine const &) noexcept = delete;
    subroutine &operator=(subroutine const &other) noexcept = delete;

    subroutine(subroutine &&other) noexcept {
        this = std::move(other);
    }

    subroutine &operator=(subroutine &&other) noexcept {
        if (this != &other) {
            std::swap(h_, other.h_);
        }
        return *this;
    }

    bool await_ready() {
        if (h_)
            return h_.done();
        return true;
    }

    bool await_suspend(std::coroutine_handle<>) {
        call_stack.push(h_);
        return true;
    }

    T await_resume() {
        if (h_.promise().eptr) {
            std::rethrow_exception(h_.promise().eptr);
        }
        return h_.promise().ret;
    }

    void resume() {
        call_stack.push(h_);
        while (!call_stack.empty()) {
            call_stack.top().resume();
            fmt::print("stopped\n");
        }
    }

    ~subroutine() {
        if (h_)
            h_.destroy();
    }

    struct promise_type {
        std::coroutine_handle<promise_type> h = nullptr;
        T ret = T{};
        std::exception_ptr eptr = nullptr;

        promise_type() {
            h = std::coroutine_handle<promise_type>::from_promise(*this);
        }

        subroutine get_return_object() noexcept {
            return subroutine(h);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept {
            struct awaitable {
                bool await_ready() const noexcept { return false; }
                void await_suspend(std::coroutine_handle<>) const noexcept {
                    call_stack.pop();
                }
                void await_resume() const noexcept {  }
            };
            return awaitable{};
        }

        auto unhandled_exception() {
            eptr = std::current_exception();
        }

        void return_value(T t) {
            ret = t;
        }
    };

private:
    std::coroutine_handle<promise_type> h_ = nullptr;

    explicit
    subroutine(std::coroutine_handle<promise_type> h) noexcept: h_{h} {
    }
};

/*

subroutine<int> f0() {
    fmt::print("f0A!\n");
    co_await std::suspend_always{};
    fmt::print("f0B!\n");
    co_return 10;
}

subroutine<int> f1() {
    fmt::print("f1A!\n");
    co_await f0();
    fmt::print("f1B!\n");
    co_return 10;
}

subroutine<int> f2() {
    int x = co_await f1();
    for (int i = 0; i < 2; ++i)
        co_await f1();
    fmt::print("i = {}\n", x);
    co_return 5;
}

*/

subroutine<int> raise() {
    fmt::print("raiseA!\n");
    throw std::runtime_error("test");
    fmt::print("raiseB!\n");
    co_return 0;
}

subroutine<int> f0() {
    fmt::print("f0A!\n");
    co_await std::suspend_always{};
    fmt::print("f0B!\n");
    try {
        co_await raise();
    }
    catch (std::runtime_error &e) {
        fmt::print("error: {}\n", e.what());
    }
    fmt::print("f0C!\n");
    co_return 10;
}

subroutine<int> f1() {
    fmt::print("f1A!\n");
    co_await f0();
    fmt::print("f1B!\n");
    co_return 10;
}

int main() {
    auto f = f1();
    f.resume();
    return 0;
}
