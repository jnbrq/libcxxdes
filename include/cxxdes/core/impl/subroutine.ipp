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
