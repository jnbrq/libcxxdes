template <typename Condition>
struct any_all_helper {
    struct custom_handler: token_handler, Condition {
        std::size_t total = 0;
        std::size_t remaining = 0;
        memory::ptr<token> completion_tkn = nullptr;
        environment *env = nullptr;

        void invoke(token *tkn) override {
            --remaining;
        
            if (tkn->eptr) {
                // in case an argument of all/any of throws an exception
                // this must be handled by main.
                std::rethrow_exception(tkn->eptr);
            }
            
            if (completion_tkn && Condition::operator()(total, remaining)) {
                // inherit the output_event features
                completion_tkn->time += tkn->time;
                completion_tkn->priority = tkn->priority;
                completion_tkn->coro_data = tkn->coro_data;
                env->schedule_token(completion_tkn.get());
                completion_tkn = nullptr;
            }
        }
    };

    template <typename Derived>
    struct base: Condition {
        auto &priority(priority_type priority) noexcept {
            priority_ = priority;
            return *this;
        }

        auto &return_latency(time_integral latency) noexcept {
            latency_ = latency;
            return *this;
        }

        void await_bind(environment *env, priority_type priority) {
            env_ = env;

            if (priority_ == priority_consts::inherit)
                priority_ = priority;
            
            derived().apply([&](auto &a) { a.await_bind(env, priority_); });
        }

        bool await_ready() {
            auto total = derived().count();
            remaining_ = total;
            derived().apply([&](auto &a) mutable { remaining_ -= (a.await_ready() ? 1 : 0); });
            return Condition::operator()(total, remaining_);
        }

        void await_suspend(coroutine_data_ptr coro_data) {
            tkn_ = new token(latency_, priority_, coro_data);

            auto handler = new custom_handler;
            handler->total = derived().count();
            handler->remaining = remaining_;
            handler->env = env_;
            handler->completion_tkn = tkn_;

            derived().apply([&](auto &a) {
                a.await_suspend(coro_data);
                if (a.await_token())
                    a.await_token()->handler = handler;
            });
        }

        token *await_token() const noexcept {
            return tkn_;
        }

        void await_resume(no_return_value_tag = {}) {
            derived().apply([&](auto &a) { a.await_resume(no_return_value_tag{}); });
        }
    private:
        std::size_t remaining_ = 0;

        auto derived() -> auto & {
            return *(static_cast<Derived *>(this));
        }

        auto derived() const -> auto const & {
            return *(static_cast<Derived const *>(this));
        }
        
        environment *env_ = nullptr;
        token *tkn_ = nullptr;
        time_integral latency_ = 0;
        priority_type priority_ = priority_consts::inherit;
    };

    template <awaitable ...As>
    struct tuple_based: std::tuple<As...>, base<tuple_based<As...>> {
        using std::tuple<As...>::tuple;
        
        constexpr std::size_t count() const noexcept {
            return sizeof...(As);
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::apply([&](As & ...as) { (f(as), ...); }, (std::tuple<As...> &)(*this));
        }
    };

    template <typename Iterator>
    struct range_based: base<range_based<Iterator>> {
        Iterator first, last;
        std::size_t size;

        range_based(Iterator first_, Iterator last_, std::size_t size_):
            first{first_}, last{last_}, size{size_} {
            // see the note below
        }
        
        constexpr std::size_t count() const noexcept {
            return size;
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::for_each(first, last, f);
        }
    };

    template <typename ValueType>
    struct vector_based: base<vector_based<ValueType>> {
        std::vector<ValueType> v;

        template <typename Iterator>
        vector_based(Iterator begin, Iterator end): v(begin, end) {
        }
        
        constexpr std::size_t count() const noexcept {
            return v.size();
        }

        template <typename UnaryFunction>
        void apply(UnaryFunction f) {
            std::for_each(v.begin(), v.end(), f);
        }
    };

    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await any_of(awaitables...) or all_of(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return by_value(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.by_value(awaitables...)")]]
        constexpr auto by_value(Ts && ...ts) const {
            return tuple_based<std::remove_cvref_t<Ts>...>{ std::forward<Ts>(ts)... };
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.by_reference(awaitables...)")]]
        constexpr auto by_reference(Ts && ...ts) const {
            return tuple_based<Ts &&...>{ std::forward<Ts>(ts)... };
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await {any_of, all_of}.copy_rvalues(awaitables...)")]]
        constexpr auto copy_rvalues(Ts && ...ts) const {
            return tuple_based<Ts...>{ std::forward<Ts>(ts)... };
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return range_copy(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range_copy(begin, end)")]]
        constexpr auto range_copy(Iterator first, Iterator last) const {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            return vector_based<value_type>(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await {any_of, all_of}.range_no_copy(begin, end)")]]
        constexpr auto range_no_copy(Iterator first, Iterator last) const {
            // -Werror=missing-field-initializers
            // we cannot use the following due to a GCC bug
            /*
            return range_based<Iterator>{
                .first = first,
                .last = last,
                .size = (std::size_t) std::distance(first, last)
            };
            */
            return range_based<Iterator>(first, last, (std::size_t) std::distance(first, last));
        }
    };
};

struct any_of_condition {
    constexpr bool operator()(std::size_t total, std::size_t remaining) const {
        return remaining < total;
    }
};

struct all_of_condition {
    constexpr bool operator()(std::size_t /* total */, std::size_t remaining) const {
        return remaining == 0;
    }
};

inline constexpr any_all_helper<any_of_condition>::functor any_of;
inline constexpr any_all_helper<all_of_condition>::functor all_of;
