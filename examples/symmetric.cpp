#include <coroutine>
#include <fmt/core.h>

template <typename T = int>
struct task {
    struct promise_type;

    task() noexcept: h_{nullptr} {  }

    task(task const &) noexcept = delete;
    task &operator=(task const &other) noexcept = delete;

    task(task &&other) noexcept {
        this = std::move(other);
    }

    task &operator=(task &&other) noexcept {
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

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> prev) {
        h_.promise().prev = prev;
        return h_;
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

    ~task() {
        if (h_)
            h_.destroy();
    }

    struct promise_type {
        std::coroutine_handle<promise_type> h = nullptr;
        std::coroutine_handle<> prev = nullptr;
        T ret = T{};
        std::exception_ptr eptr = nullptr;

        promise_type() {
            h = std::coroutine_handle<promise_type>::from_promise(*this);
        }

        task get_return_object() noexcept {
            return task(h);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept {
            struct final_awaitable {
                std::coroutine_handle<> h;

                bool await_ready() noexcept {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                    if (not h)
                        return std::noop_coroutine();
                    return h;
                }

                void await_resume() noexcept {  }
            };

            return final_awaitable{prev};
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
    task(std::coroutine_handle<promise_type> h) noexcept: h_{h} {
    }
};


task<int> f1() {
    co_return 10;
}

task<int> f2() {
    int x = co_await f1();
    fmt::print("i = {}\n", x);
    co_return 5;
}

int main() {
    auto f = f2();
    f.resume();
    return 0;
}
