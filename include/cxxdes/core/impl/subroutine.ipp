/**
 * @brief Coroutine helper that runs inside the currently scheduled process.
 *
 * Unlike `coroutine<T>`, a subroutine is not an independently scheduled process.
 * Awaiting it pushes its coroutine frame onto the current process call stack and
 * execution continues within the same `coroutine_data`.
 *
 * @tparam ReturnType Value returned to the caller, or `void`.
 */
template <typename ReturnType = void>
struct subroutine {
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

    /** @brief Returns true if there is no frame to run or the frame is done. */
    bool await_ready() {
        if (h_) return h_.done();
        return true;
    }

    /** @brief Pushes this subroutine frame onto the current coroutine stack. */
    void await_suspend(std::coroutine_handle<>) {
        auto &promise = h_.promise();
        promise.coro_data->push_coro_(h_);
    }

    /**
     * @brief Returns the subroutine result after completion.
     *
     * @note Exceptions thrown inside the subroutine body are rethrown here.
     */
    ReturnType await_resume() {
        auto &promise = h_.promise();
        if (promise.eptr)
            std::rethrow_exception(promise.eptr);
        
        if constexpr (not std::is_same_v<ReturnType, void>)
            return std::move(*promise.ret);
    }

    /** @brief Wraps this subroutine as an independently schedulable coroutine. */
    unique_coroutine<ReturnType> as_coroutine() &&;

    ~subroutine() {
        if (h_) h_.destroy();
    }

private:
    template <typename>
    friend struct detail::await_ops_mixin;

    void bind_coroutine_(coroutine_data_ptr coro_data) {
        auto &promise = h_.promise();
        promise.coro_data = std::move(coro_data);
    }

    template <typename Derived>
    struct return_value_mixin {
        std::optional<ReturnType> ret;

        template <typename T>
        void return_value(T &&t) {
            ret.emplace(std::forward<T>(t));
        }
    };

    template <typename Derived>
    struct return_void_mixin {
        void return_void() {
        }
    };

public:
    struct promise_type:
        std::conditional_t<
            std::is_same_v<ReturnType, void>,
            return_void_mixin<promise_type>,
            return_value_mixin<promise_type>
        >, detail::await_ops_mixin<promise_type> {
        template <typename>
        friend struct subroutine;

        std::coroutine_handle<promise_type> h = nullptr;
        std::exception_ptr eptr = nullptr;
        coroutine_data_ptr coro_data = nullptr;

        promise_type() {
            h = std::coroutine_handle<promise_type>::from_promise(*this);
        }

        subroutine get_return_object() noexcept {
            return subroutine(h);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept {
            struct final_awaitable {
                coroutine_data_ptr coro_data;

                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<>) noexcept {
                    coro_data->pop_coro_();
                }
                void await_resume() noexcept {  }
            };
            return final_awaitable{coro_data};
        }

        auto unhandled_exception() {
            eptr = std::current_exception();
            // after this, will call final_suspend() and finalize
        }
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
    a.bind_coroutine_(derived().coro_data.get());
    return std::move(a);
}

} /* namespace detail */
