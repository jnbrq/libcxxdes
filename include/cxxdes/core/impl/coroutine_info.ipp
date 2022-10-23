struct coroutine_info: memory::reference_counted_base<coroutine_info> {
    coroutine_info(util::source_location created):
        created_{created} {
    }

    coroutine_info(const coroutine_info &) = delete;
    coroutine_info &operator=(const coroutine_info &) = delete;

    coroutine_info(coroutine_info &&) = delete;
    coroutine_info &operator=(coroutine_info &&) = delete;

    util::source_location const &loc_created() const noexcept {
        return created_;
    }

    util::source_location const &loc_awaited() const noexcept {
        return awaited_;
    }

    void resume() {
        if (complete_)
            // a token might try to resume an interrupted coroutine
            return ;

        // note that coroutine<> does not pop the first coroutine
        do {
            should_continue_ = false;
            auto top = call_stack_.top();
            top.resume();
        } while (should_continue_);
    }

    [[nodiscard]]
    environment *env() noexcept {
        return env_;
    }

    [[nodiscard]]
    environment const *env() const noexcept {
        return env_;
    }

    [[nodiscard]]
    coroutine_info_ptr parent() noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    const_coroutine_info_ptr parent() const noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    priority_type priority() const noexcept {
        return priority_;
    }

    void priority(priority_type priority) noexcept {
        priority_ = priority;
    }

    [[nodiscard]]
    time_integral latency() const noexcept {
        return latency_;
    }

    void latency(time_integral latency) noexcept {
        latency_ = latency;
    }

    [[nodiscard]]
    bool started() const noexcept {
        return env_ != nullptr;
    }

    [[nodiscard]]
    bool complete() const noexcept {
        return complete_;
    }

    template <typename T = interrupted_exception>
    void interrupt(T &&t = interrupted_exception{}) noexcept {
        exception_.assign(std::forward<T>(t));
    }

    [[nodiscard]]
    bool interrupted() const noexcept {
        return exception_.valid();
    }

    virtual ~coroutine_info() = default;

protected:
    template <awaitable A>
    friend struct awaitable_wrapper;

    template <typename ReturnType, bool Unique>
    friend struct coroutine;

    template <typename ReturnType>
    friend struct subroutine;

    void bind_(environment *env, priority_type priority);
    void manage_();
    void unmanage_();

    void raise_interrupt_() {
        // clear exception_, the exception might be caught
        // we may need to interrupt again later
        auto exception = std::move(exception_);
        exception.raise();
    }

    void push_coro_(coroutine_handle coro) {
        // whenever there is a subroutine call, the
        // execution should keep moving further
        call_stack_.push(coro);
        should_continue_ = true;
    }

    void pop_coro_() {
        // whenever a subroutine call returns, the
        // execution should keep moving further
        call_stack_.pop();
        should_continue_ = true;
    }

    void destroy_if_not_started_() {
        if (ref_count() == 2 && !started()) {
            // if called, only two owners: (1) coroutine<T> and (2) promise_type
            // as the coroutine is not started yet, promise_type will never
            // get destroyed, resulting in a memory leak
            call_stack_.top().destroy();
        }
    }

protected:
    environment *env_ = nullptr;
    std::stack<coroutine_handle> call_stack_;
    bool should_continue_ = false;
    util::source_location created_;
    util::source_location awaited_;
    priority_type priority_ = priority_consts::inherit;
    time_integral latency_ = 0;
    memory::ptr<coroutine_info> parent_;
    bool complete_ = false;
    exception_container exception_;
};


namespace detail {

template <typename ReturnType = void>
struct coroutine_info_return_value_mixin {
    using return_type = ReturnType;

    template <typename ...Args>
    void emplace_return_value(Args && ...args) {
        return_value_.emplace(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    bool has_return_value() const noexcept {
        return return_value_.has_value();
    }
    
    [[nodiscard]]
    ReturnType &return_value() noexcept {
        return *return_value_;
    }

    [[nodiscard]]
    ReturnType const &return_value() const noexcept {
        return *return_value_;
    }
protected:
    std::optional<ReturnType> return_value_;
};

template <>
struct coroutine_info_return_value_mixin<void> {
    using return_type = void;
};

template <bool Unique = false>
struct coroutine_info_completion_tokens_mixin {
    explicit coroutine_info_completion_tokens_mixin() {
        completion_tokens_.reserve(2);
    }

    void completion_token(token *completion_token) {
        completion_tokens_.push_back(completion_token);
    }

protected:
    void schedule_completion_(environment *env);

    std::vector<memory::ptr<token>> completion_tokens_;
};

template <>
struct coroutine_info_completion_tokens_mixin<true> {
    void completion_token(token *completion_token) {
        completion_token_ = completion_token;
    }

protected:
    void schedule_completion_(environment *env);

    memory::ptr<token> completion_token_ = nullptr;
};

} /* namespace detail */


template <typename ReturnType = void, bool Unique = false>
struct coroutine_info_:
    coroutine_info,
    detail::coroutine_info_completion_tokens_mixin<Unique>,
    detail::coroutine_info_return_value_mixin<ReturnType> {
    using coroutine_info::coroutine_info;

    void do_return();

    virtual ~coroutine_info_() = default;
};