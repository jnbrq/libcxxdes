template <typename Derived, bool Timeout = true>
struct timeout_base {
    constexpr timeout_base(priority_type priority = priority_consts::inherit): priority_{priority} {
    }

    void await_bind(environment *env, priority_type priority) noexcept {
        env_ = env;

        if (priority_ == priority_consts::inherit) {
            priority_ = priority;
        }
    }

    bool await_ready() const noexcept {
        if constexpr (Timeout)
            return false;
        else
            return env().now() >= derived().time();
    }

    void await_suspend(coroutine_data_ptr coro_data) {
        time_integral pt = 0;

        if constexpr (Timeout) {
            pt = env().now() + derived().time();
        }
        else {
            pt = derived().time();
        }

        env_->schedule_token(tkn_ = new token(pt, priority_, coro_data, "timeout"));
    }

    token *await_token() const noexcept {
        return tkn_;
    }

    void await_resume(no_return_value_tag = {}) {
        tkn_ = nullptr;
    }

    auto &env() const noexcept {
        return *env_;
    }

protected:
    environment *env_ = nullptr;
    token *tkn_ = nullptr;

    priority_type priority_;

    auto derived() const noexcept -> auto const & {
        return static_cast<Derived const &>(*this);
    }
};

template <bool Timeout>
struct timeout_functor {
    template <cxxdes::time_utils::node T>
    constexpr auto operator()(T &&t, priority_type priority = priority_consts::inherit) const noexcept {
        struct [[nodiscard]] result: timeout_base<result, Timeout> {
            using base = timeout_base<result, Timeout>;

            std::remove_cvref_t<T> t;

            time_integral time() const noexcept {
                return base::env().real_to_sim(t);
            }
        };

        return result{ { priority }, std::forward<T>(t) };
    }

    constexpr auto operator()(time_integral t, priority_type priority = priority_consts::inherit) const noexcept {
        struct [[nodiscard]] result: timeout_base<result, Timeout> {
            time_integral t;

            time_integral time() const noexcept {
                return t;
            }
        };

        return result{ { priority }, t };
    }
};

inline constexpr timeout_functor<true> timeout, delay;
inline constexpr timeout_functor<false> instant, until;

template <typename Derived>
struct lazy_timeout_base {
    constexpr lazy_timeout_base(priority_type priority = priority_consts::inherit): priority_{priority} {
    }

    bool await_ready() const noexcept {
        return true;
    }

    void await_suspend(coroutine_data_ptr) const noexcept {  }

    token *await_token() const noexcept { return nullptr; }

    auto await_resume() const noexcept {
        return instant(derived().time(), priority_);
    }

    void await_resume(no_return_value_tag) {  }

protected:
    priority_type priority_;

    auto derived() const noexcept -> const auto & {
        return static_cast<Derived const &>(*this);
    }
};

struct lazy_timeout_functor {
    template <typename T>
    constexpr auto operator()(T &&t, priority_type priority = priority_consts::inherit) const noexcept {
        struct [[nodiscard]] result: lazy_timeout_base<result> {
            std::remove_cvref_t<T> t;
            time_integral tsim = 0;

            void await_bind(environment *env, priority_type) noexcept {
                tsim = env->now() + env->real_to_sim(t);
            }

            time_integral time() const noexcept {
                return tsim;
            }
        };

        return result{ { priority }, std::forward<T>(t)};
    }

    constexpr auto operator()(time_integral t, priority_type priority = priority_consts::inherit) const noexcept {
        struct [[nodiscard]] result: lazy_timeout_base<result> {
            time_integral t;
            time_integral tsim = 0;

            void await_bind(environment *env, priority_type) noexcept {
                tsim = env->now() + t;
            }

            time_integral time() const noexcept {
                return tsim;
            }
        };

        return result{ { priority }, t };
    }
};

inline constexpr lazy_timeout_functor lazy_timeout, lazy_delay;

inline constexpr auto yield() noexcept {
    return delay(0);
}

template <typename T>
inline auto environment::timeout(T &&t) const noexcept {
    return delay(real_to_sim(t));
}
