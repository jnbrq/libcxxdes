struct environment {
    environment(time const &unit = one_second, time const &prec = one_second):
        now_{(time_integral) 0}, unit_{unit}, prec_{prec} {
    }

    time_integral now() const noexcept {
        return now_;
    }

    time t() const noexcept {
        return { now() * prec_.t, prec_.u };
    }

    real_type now_seconds() const noexcept {
        return t().seconds<real_type>();
    }

    void time_unit(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_unit(time x) on a used environment!");
        
        unit_ = x;
    }

    time time_unit() const noexcept {
        return unit_;
    }

    void time_precision(time x) {
        if (used_)
            throw std::runtime_error("cannot call time_precision(time x) on a used environment!");
        
        prec_ = x;
    }

    time time_precision() const noexcept {
        return prec_;
    }

    template <cxxdes::time_utils::node Node>
    time_integral real_to_sim(Node const &n) const noexcept {
        return n.count(time_precision(), time_unit());
    }

    template <cxxdes::time_utils::scalar Scalar>
    time_integral real_to_sim(Scalar const &s) const noexcept {
        return unitless_time<Scalar>{s}.count(time_precision(), time_unit());
    }

    template <typename T>
    auto timeout(T &&t) const noexcept;

    void schedule_token(token *tkn) {
        used_ = true;
        tkn->ref();
        tokens_.push(tkn);
    }

    [[nodiscard]]
    token *next_event() const noexcept {
        if (tokens_.size() > 0)
            return tokens_.top();
        return nullptr;
    }

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

    auto &run() {
        while (step());
        return *this;
    }
    
    auto &run_until(time_integral t) {
        while (now() <= t && step()) ;
        return *this;
    }

    auto &run_until(time_expr t) {
        run_until(real_to_sim(t));
        return *this;
    }

    auto &run_for(time_integral t) {
        run_until(now() + t);
        return *this;
    }

    auto &run_for(time_expr t) {
        run_until(now() + real_to_sim(t));
        return *this;
    }

    template <typename ReturnType, bool Unique>
    void bind(coroutine<ReturnType, Unique> p);

    coroutine_data_ptr current_coroutine() const noexcept {
        return current_coroutine_;
    }

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
