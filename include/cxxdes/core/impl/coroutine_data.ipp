struct coroutine_data: memory::reference_counted_base<coroutine_data> {
    coroutine_data(util::source_location created):
        created_{created} {
    }

    coroutine_data(const coroutine_data &) = delete;
    coroutine_data &operator=(const coroutine_data &) = delete;

    coroutine_data(coroutine_data &&) = delete;
    coroutine_data &operator=(coroutine_data &&) = delete;

    util::source_location const &loc_created() const noexcept {
        return created_;
    }

    util::source_location const &loc_awaited() const noexcept {
        return awaited_;
    }

    void resume() {
        assert(!complete_);

        // note that coroutine<> does not pop the first coroutine
        do {
            should_continue_ = false;
            auto top = call_stack_.back();
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
    coroutine_data_ptr parent() noexcept {
        return parent_.get();
    }

    [[nodiscard]]
    const_coroutine_data_ptr parent() const noexcept {
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

    [[nodiscard]]
    bool bound() const noexcept {
        return env_ != nullptr;
    }

    void kill() {
        while (!call_stack_.empty()) {
            call_stack_.back().destroy();
            call_stack_.pop_back();
        }
    }

    virtual ~coroutine_data() = default;

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
    
    void push_coro_(coroutine_handle coro) {
        // whenever there is a subroutine call, the
        // execution should keep moving further
        call_stack_.push_back(coro);
        should_continue_ = true;
    }

    void pop_coro_() {
        // whenever a subroutine call returns, the
        // execution should keep moving further
        call_stack_.pop_back();
        should_continue_ = true;
    }

    void destroy_if_not_started_() {
        if (ref_count() == 2 && !started()) {
            // if called, only two owners: (1) coroutine<T> and (2) promise_type
            // as the coroutine is not started yet, promise_type will never
            // get destroyed, resulting in a memory leak
            call_stack_.back().destroy();
        }
    }

protected:
    environment *env_ = nullptr;
    std::vector<coroutine_handle> call_stack_;
    bool should_continue_ = false;
    util::source_location created_;
    util::source_location awaited_;
    priority_type priority_ = priority_consts::inherit;
    time_integral latency_ = 0;
    memory::ptr<coroutine_data> parent_;
    bool complete_ = false;
};


namespace detail {

template <typename ReturnType = void>
struct coroutine_data_return_value_mixin {
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
struct coroutine_data_return_value_mixin<void> {
    using return_type = void;
};

template <bool Unique = false>
struct coroutine_data_completion_tokens_mixin {
    explicit coroutine_data_completion_tokens_mixin() {
        completion_tokens_.reserve(2);
    }

    void completion_token(token *completion_token) {
        completion_tokens_.push_back(completion_token);
    }

    void propagate_exception(std::exception_ptr exception) {
        for (auto completion_token: completion_tokens_)
            completion_token->eptr = exception;
    }

protected:
    void schedule_completion_(environment *env);

    std::vector<memory::ptr<token>> completion_tokens_;
};

template <>
struct coroutine_data_completion_tokens_mixin<true> {
    void completion_token(token *completion_token) {
        completion_token_ = completion_token;
    }

    void propagate_exception(std::exception_ptr exception) {
        completion_token_->eptr = exception;
    }

protected:
    void schedule_completion_(environment *env);

    memory::ptr<token> completion_token_ = nullptr;
};

} /* namespace detail */


template <typename ReturnType = void, bool Unique = false>
struct coroutine_data_:
    coroutine_data,
    detail::coroutine_data_completion_tokens_mixin<Unique>,
    detail::coroutine_data_return_value_mixin<ReturnType> {
    using coroutine_data::coroutine_data;

    void do_return();

    virtual ~coroutine_data_() = default;
};
