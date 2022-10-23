namespace detail {

template <typename Derived, typename ReturnValue>
struct coroutine_return_value_mixin {
    [[nodiscard]]
    ReturnValue const &return_value() const noexcept {
        return static_cast<Derived const *>(this)->cinfo()->return_value();
    }
};

template <typename Derived>
struct coroutine_return_value_mixin<Derived, void> {
};

} /* namespace detail */

template <typename ReturnType = void, bool Unique = false>
struct coroutine:
    detail::coroutine_return_value_mixin<coroutine<ReturnType, Unique>, ReturnType> {
    using coroutine_info_type = coroutine_info_<ReturnType, Unique>;

    explicit
    coroutine(memory::ptr<coroutine_info_type> cinfo = nullptr):
        cinfo_{std::move(cinfo)} {
    }

    coroutine(coroutine const &other) requires (not Unique) {
        *this = other;
    }

    coroutine &operator=(coroutine const &other) requires (not Unique) {
        if (this != &other) {
            cinfo_ = other.cinfo_;
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
            cinfo_ = std::move(other.cinfo_);
            std::swap(completion_token_, other.completion_token_);
            std::swap(return_, return_);
        }
        return *this;
    }

    ~coroutine() {
        if (cinfo_) cinfo_->destroy_if_not_started_();
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return cinfo_;
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    coroutine_info_type const *cinfo() const noexcept {
        return cinfo_.get();
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return cinfo_->complete();
    }

    template <typename T = interrupted_exception>
    void interrupt(T &&t = interrupted_exception{}) noexcept {
        cinfo_->interrupt(std::forward<T>(t));
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return cinfo_->interrupted();
    }

    [[nodiscard]]
    priority_type priority() const noexcept {
        return cinfo_->priority();
    }

    auto &priority(priority_type priority) noexcept {
        cinfo_->priority(priority);
        return *this;
    }

    [[nodiscard]]
    time_integral latency() const noexcept {
        return cinfo_->latency();
    }

    auto &latency(time_integral latency) noexcept {
        cinfo_->latency(latency);
        return *this;
    }

    [[nodiscard]]
    priority_type return_priority() const noexcept {
        return return_.priority;
    }

    auto &return_priority(priority_type priority) noexcept {
        return_.priority = priority;
        return *this;
    }

    [[nodiscard]]
    time_integral return_latency() const noexcept {
        return return_.latency;
    }

    auto &return_latency(time_integral latency) noexcept {
        return_.latency = latency;
        return *this;
    }

    void await_bind(environment *env, priority_type priority = 0) {
        cinfo_->bind_(env, priority);
    }

    bool await_ready() const noexcept {
        return cinfo_->complete();
    }

    void await_suspend(coroutine_info_ptr phandle) {
        if (completion_token_)
            throw std::runtime_error("coroutine<> is already being awaited!");

        completion_token_ = new token{return_.latency, return_.priority, phandle};
        if (completion_token_->priority == priority_consts::inherit)
            completion_token_->priority = cinfo_->priority_;
        cinfo_->completion_token(completion_token_);
    }

    token *await_token() const noexcept {
        return completion_token_;
    }

    ReturnType await_resume() requires (not Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!cinfo_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return cinfo_->return_value();
        }
    }

    ReturnType await_resume() requires (Unique) {
        await_resume(no_return_value_tag{});

        if constexpr (std::is_same_v<ReturnType, void>)
            return ;
        else {
            if (!cinfo_->has_return_value())
                throw std::runtime_error("no return value from the coroutine<T> [T != void]!");

            return std::move(cinfo_->return_value());
        }
    }

    void await_resume(no_return_value_tag) noexcept {
        completion_token_ = nullptr;
    }

private:
    memory::ptr<coroutine_info_type> cinfo_ = nullptr;
    token *completion_token_ = nullptr; // must be non-owning, in case of copies

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
        memory::ptr<coroutine_info_type> cinfo;
    
        template <typename ...Args>
        promise_type(Args && ...args) {
            auto loc = util::extract_first_type<util::source_location>(args...);
            auto coro = std::coroutine_handle<promise_type>::from_promise(*this);
            cinfo = new coroutine_info_type(loc);
            cinfo->push_coro_(coro);
        }

        coroutine get_return_object() {
            return coroutine(cinfo);
        }

        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> std::suspend_never { return {}; }

        auto unhandled_exception() -> void {
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (stopped_exception & /* ex */) {
                do_return();
            }
            catch (...) {
                std::rethrow_exception(std::current_exception());
            }
        }

        template <typename T>
        void emplace_return_value(T &&t) {
            cinfo->emplace_return_value(std::forward<T>(t));
        }

        void do_return() {
            cinfo->do_return();
        }

        ~promise_type() {
        }
    };
};

template <typename ReturnValue = void>
using unique_coroutine = coroutine<ReturnValue, true>;
