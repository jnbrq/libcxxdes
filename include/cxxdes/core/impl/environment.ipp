/**
 * @brief Owns simulation time and the scheduled event queue.
 *
 * An environment binds coroutine processes, schedules their resume tokens, and
 * advances simulated time by executing tokens in timestamp/priority order. Time
 * values passed as time expressions are converted using the configured
 * `time_unit()` and `time_precision()`.
 *
 * The environment owns scheduled coroutine state after binding. It is not
 * thread-safe.
 */
struct environment {
    /**
     * @brief Constructs an empty environment.
     *
     * @param unit Real-world unit represented by a unitless model value.
     * @param prec Real-world duration represented by one simulation tick.
     */
    environment(time const &unit = one_second, time const &prec = one_second):
        now_{(time_integral) 0}, unit_{unit}, prec_{prec} {
    }

    /** @brief Returns the current simulation timestamp in precision ticks. */
    time_integral now() const noexcept {
        return now_;
    }

    /** @brief Returns the current simulation time as a physical time quantity. */
    time t() const noexcept {
        return { now() * prec_.t, prec_.u };
    }

    /** @brief Returns the current simulation time converted to seconds. */
    real_type now_seconds() const noexcept {
        return t().seconds<real_type>();
    }

    /**
     * @brief Sets the model unit used to convert unitless time expressions.
     *
     * @throws std::runtime_error If any token has already been scheduled.
     */
    void time_unit(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_unit(time x) on a used environment!");
        
        unit_ = x;
    }

    /** @brief Returns the model unit used for unitless time expressions. */
    time time_unit() const noexcept {
        return unit_;
    }

    /**
     * @brief Sets the physical duration represented by one simulation tick.
     *
     * @throws std::runtime_error If any token has already been scheduled.
     */
    void time_precision(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_precision(time x) on a used environment!");
        
        prec_ = x;
    }

    /** @brief Returns the physical duration represented by one simulation tick. */
    time time_precision() const noexcept {
        return prec_;
    }

    /** @brief Converts a time-expression node to simulation ticks. */
    template <cxxdes::time_utils::node Node>
    time_integral real_to_sim(Node const &n) const noexcept {
        return n.count(time_precision(), time_unit());
    }

    /** @brief Converts a scalar model value to simulation ticks using `time_unit()`. */
    template <cxxdes::time_utils::scalar Scalar>
    time_integral real_to_sim(Scalar const &s) const noexcept {
        return unitless_time<Scalar>{s}.count(time_precision(), time_unit());
    }

    /** @brief Returns a relative timeout awaitable after converting @p t to ticks. */
    template <typename T>
    auto timeout(T &&t) const noexcept;

    /**
     * @brief Schedules a resume token.
     *
     * The environment takes a reference to @p tkn until the token is popped or
     * cleared by `reset()`.
     */
    void schedule_token(token *tkn) {
        used_ = true;
        tkn->ref();
        tokens_.push(tkn);
    }

    /** @brief Returns the next scheduled token without removing it, or null. */
    [[nodiscard]]
    token *next_event() const noexcept {
        if (tokens_.size() > 0)
            return tokens_.top();
        return nullptr;
    }

    /**
     * @brief Executes the next scheduled event.
     *
     * @retval true A token was popped and processed.
     * @retval false No token was available.
     *
     * @note Exceptions propagated by token handlers or exception tokens are
     *       rethrown to the caller.
     */
    bool step() {
        if (tokens_.empty())
            return false;
        
        auto tkn = memory::ptr{tokens_.top()};
        tkn->unref() /* tkn already holds a reference now */;
        tkn->attempt_access();
        tokens_.pop();

        now_ = std::max(tkn->time, now_);

        if (tkn->handler) {
            try {
                tkn->handler->invoke(tkn);
            }
            catch (...) {
                std::rethrow_exception(std::current_exception());
            }
        }
        else if (tkn->coro_data) {
            current_coroutine_ = tkn->coro_data;
            tkn->coro_data->resume();
            current_coroutine_ = nullptr;
        }
        else if (tkn->eptr) {
            std::rethrow_exception(tkn->eptr);
        }
        
        return true;
    }

    /**
     * @brief Destroys incomplete managed coroutines and clears scheduled tokens.
     *
     * Simulation time is reset to zero. The configured time unit and precision
     * are left unchanged.
     */
    void reset() {
        // it is not safe to iterate over the coroutines while
        // individual coroutines might actively modify the
        // unoredered_set. move from it.
        // for a proper way to erase while iterating:
        //   https://en.cppreference.com/w/cpp/container/unordered_set/erase
        // sadly, we cannot apply this solution.
        auto coroutines = std::move(coroutines_);

        for (auto coroutine: coroutines) {
            if (!coroutine->complete()) {
                coroutine->destroy();
            }
        }

        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            tkn->unref();
        }

        now_ = 0;
    }

    /** @brief Runs scheduled events until the queue is empty. */
    auto &run() {
        while (step());
        return *this;
    }
    
    /**
     * @brief Runs all events scheduled at or before @p t.
     *
     * If the next event is later than @p t, it remains scheduled and time still
     * advances to @p t.
     */
    auto &run_until(time_integral t) {
        while (next_event() && next_event()->time <= t)
            step();

        now_ = std::max(now_, t);
        return *this;
    }

    /** @brief Converts @p t to ticks and runs until that absolute timestamp. */
    auto &run_until(time_expr t) {
        run_until(real_to_sim(t));
        return *this;
    }

    /** @brief Runs for @p t simulation ticks relative to `now()`. */
    auto &run_for(time_integral t) {
        run_until(now() + t);
        return *this;
    }

    /** @brief Converts @p t to ticks and runs for that relative duration. */
    auto &run_for(time_expr t) {
        run_until(now() + real_to_sim(t));
        return *this;
    }

    /**
     * @brief Binds a coroutine process to this environment.
     *
     * Binding schedules the process start token if the process has not already
     * been bound. Rebinding the same process to another environment throws.
     */
    template <typename ReturnType, bool Unique>
    void bind(coroutine<ReturnType, Unique> p);

    /** @brief Returns the coroutine currently being resumed, or null. */
    coroutine_data_ptr current_coroutine() const noexcept {
        return current_coroutine_;
    }

    /** @brief Returns the source location recorded by the current await transform. */
    [[nodiscard]]
    util::source_location const &loc() const noexcept {
        return loc_;
    }

    ~environment() {
        reset();
    }

private:
    time_integral now_ = 0;
    bool used_ = false;

    time unit_;
    time prec_;

    struct token_comp {
        static bool has_priority(token *tkn) {
            // it has an exception.
            // this token can only be processed by step(), because
            // no coroutine<> or handler object reference it.
            return tkn->coro_data == nullptr && tkn->handler == nullptr && tkn->eptr != nullptr;
        }

        bool operator()(token *tkn_a, token *tkn_b) const {
            return
                has_priority(tkn_b) ||
                (tkn_a->time > tkn_b->time) ||
                (tkn_a->time == tkn_b->time && tkn_a->priority > tkn_b->priority);
        }
    };

    std::priority_queue<token *, std::vector<token *>, token_comp> tokens_;
    
    friend struct coroutine_data;

    template <typename ReturnType, bool Unique>
    friend struct coroutine;

    template <typename ReturnType>
    friend struct subroutine;

    template <typename Derived>
    friend struct detail::await_ops_mixin;

    std::unordered_set<memory::ptr<coroutine_data>> coroutines_;
    coroutine_data_ptr current_coroutine_ = nullptr;
    util::source_location loc_;
};

inline
void coroutine_data::bind_(environment *env, priority_type priority) {
    if (env_) {
        if (env_ != env)
            throw std::runtime_error("cannot bind an already bound coroutine to another environment.");
        
        // already started
        return ;
    }
    
    env_ = env;
    awaited_ = env_->loc();
    parent_ = env_->current_coroutine();

    if (priority_ == priority_consts::inherit)
        priority_ = priority;
    
    auto start_token = new token{
        env_->now() + latency_,
        priority_,
        this,
        "coroutine start"
    };

    env_->schedule_token(start_token);
    manage_();
}

inline
void coroutine_data::manage_() {
    env_->coroutines_.insert(this);
}

inline
void coroutine_data::unmanage_() {
    env_->coroutines_.erase(this);
}

namespace detail {

template <bool Unique>
inline
void coroutine_data_completion_tokens_mixin<Unique>::schedule_completion_(environment *env) {
    for (auto completion_token: completion_tokens_) {
        completion_token->time += env->now();
        env->schedule_token(completion_token.get());
    }
    completion_tokens_.clear();
}

// note: template<> is forbidden here
// error: template-id ‘schedule_completion_<>’ does not match any template declaration? why?
inline
void coroutine_data_completion_tokens_mixin<true>::schedule_completion_(environment *env) {
    completion_token_->time += env->now();
    env->schedule_token(completion_token_.get());
    completion_token_ = nullptr;
}

} /* namespace detail */

template <typename ReturnType, bool Unique>
inline
void coroutine_data_<ReturnType, Unique>::do_return() {
    this->schedule_completion_(env_); // this-> is a must here
    this->complete_ = true;
    this->unmanage_();
}
