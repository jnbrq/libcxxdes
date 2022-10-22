#include <coroutine>
#include <fmt/core.h>

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
        h_.resume();
        return false;
    }

    T await_resume() {
        if (h_.promise().eptr) {
            std::rethrow_exception(h_.promise().eptr);
        }
        return h_.promise().ret;
    }

    void resume() {
        h_.resume();
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
        auto final_suspend() noexcept -> std::suspend_always { return {}; }

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


subroutine<int> f1() {
    co_return 10;
}

subroutine<int> f2() {
    int x = co_await f1();
    for (int i = 0; i < 1'000'000'000; ++i)
        co_await f1();
    fmt::print("i = {}\n", x);
    co_return 5;
}

int main() {
    auto f = f2();
    f.resume();
    return 0;
}
