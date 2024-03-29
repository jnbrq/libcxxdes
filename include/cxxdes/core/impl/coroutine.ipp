namespace detail {

template <typename Derived, typename ReturnValue>
struct coroutine_return_value_mixin {
    [[nodiscard]]
    ReturnValue const &return_value() const noexcept {
        return static_cast<Derived const *>(this)->coro_data()->return_value();
    }
};

template <typename Derived>
struct coroutine_return_value_mixin<Derived, void> {
};

} /* namespace detail */

template <typename ReturnType, bool Unique>
struct coroutine:
    detail::coroutine_return_value_mixin<coroutine<ReturnType, Unique>, ReturnType> {
    using coroutine_data_type = coroutine_data_<ReturnType, Unique>;

    explicit
    coroutine(memory::ptr<coroutine_data_type> coro_data = nullptr):
        coro_data_{std::move(coro_data)} {
    }

    coroutine(coroutine const &other) requires (not Unique) {
        *this = other;
    }

    coroutine &operator=(coroutine const &other) requires (not Unique) {
        if (this != &other) {
            coro_data_ = other.coro_data_;
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
            coro_data_ = std::move(other.coro_data_);
            std::swap(completion_token_, other.completion_token_);
            std::swap(return_, other.return_);
        }
        return *this;
    }

    ~coroutine() {
        if (coro_data_) coro_data_->destroy_if_not_started_();
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return coro_data_;
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    coroutine_data_type const *coro_data() const noexcept {
        return coro_data_.get();
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return coro_data_->complete();
    }

    [[nodiscard]]
    priority_type priority() const noexcept {
        return coro_data_->priority();
    }

    auto &priority(priority_type priority) & noexcept {
        coro_data_->priority(priority);
        return *this;
    }

    auto &&priority(priority_type priority) && noexcept {
        coro_data_->priority(priority);
        return std::move(*this);
    }

    [[nodiscard]]
    time_integral latency() const noexcept {
        return coro_data_->latency();
    }

    auto &latency(time_integral latency) & noexcept {
        coro_data_->latency(latency);
        return *this;
    }

    auto &&latency(time_integral latency) && noexcept {
        coro_data_->latency(latency);
        return std::move(*this);
    }

    [[nodiscard]]
    priority_type return_priority() const noexcept {
        return return_.priority;
    }

    auto &return_priority(priority_type priority) & noexcept {
        return_.priority = priority;
        return *this;
    }

    auto &&return_priority(priority_type priority) && noexcept {
        return_.priority = priority;
        return std::move(*this);
    }

    [[nodiscard]]
    time_integral return_latency() const noexcept {
        return return_.latency;
    }

    auto &return_latency(time_integral latency) & noexcept {
        return_.latency = latency;
        return *this;
    }

    auto &&return_latency(time_integral latency) && noexcept {
        return_.latency = latency;
        return std::move(*this);
    }

    void await_bind(environment *env, priority_type priority = 0) {
        coro_data_->bind_(env, priority);
    }

    bool await_ready() const noexcept {
        return coro_data_->complete();
    }

    void await_suspend(coroutine_data_ptr coro_data) {
        if (completion_token_)
            throw std::runtime_error("coroutine<> is already being awaited!");

        completion_token_ = new token{
            return_.latency,
            return_.priority,
            coro_data,
            "coroutine completion"
        };
        if (completion_token_->priority == priority_consts::inherit)
            completion_token_->priority = coro_data_->priority_;
        coro_data_->completion_token(completion_token_);
    }

    token *await_token() {
        return completion_token_.get();
    }

    ReturnType await_resume() requires (not Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!coro_data_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return coro_data_->return_value();
        }
    }

    ReturnType await_resume() requires (Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!coro_data_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return std::move(coro_data_->return_value());
        }
    }

    void await_resume(no_return_value_tag) {
        if (completion_token_) {
            completion_token_->attempt_access();

            auto tkn = completion_token_;
            completion_token_ = nullptr;

            if (tkn->eptr) {
                std::rethrow_exception(tkn->eptr);
            }
        }
    }

private:
    memory::ptr<coroutine_data_type> coro_data_ = nullptr;
    memory::ptr<token> completion_token_ = nullptr;

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
        memory::ptr<coroutine_data_type> coro_data;
    
        template <typename ...Args>
        promise_type(Args && ...args) {
            auto loc = util::extract_first_type<util::source_location>(args...);
            auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
            coro_data = new coroutine_data_type(loc);
            coro_data->push_coro_(coro);
        }

        coroutine get_return_object() {
            return coroutine(coro_data);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }

        auto unhandled_exception() -> void {
            // update completion tokens with the current exception
            coro_data->propagate_exception(std::current_exception());

            // schedule the completion tokens
            coro_data->do_return();
        }

        template <typename T>
        void emplace_return_value(T &&t) {
            coro_data->emplace_return_value(std::forward<T>(t));
        }

        void do_return() {
            coro_data->do_return();
        }

        ~promise_type() {
        }
    };
};

template <typename ReturnType>
unique_coroutine<ReturnType> subroutine<ReturnType>::as_coroutine() && {
    return [](subroutine<ReturnType> s) -> unique_coroutine<ReturnType> {
        if constexpr (std::is_same_v<ReturnType, void>)
            co_await std::move(s);
        else {
            co_return std::move(co_await std::move(s));
        }
    }(std::move(*this));
}

template <typename ReturnType, bool Unique>
void environment::bind(coroutine<ReturnType, Unique> p) {
    p.await_bind(this, 0);
    if (!p.await_ready())
        p.await_suspend(nullptr);
}
