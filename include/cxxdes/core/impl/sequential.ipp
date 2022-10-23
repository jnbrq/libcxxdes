struct sequential_helper {
    template <typename ...Ts>
    static coroutine<void> seq_proc_tuple(Ts ...ts) {
        ((co_await std::move(ts)), ...);
        co_return ;
    }

    template <typename Iterator>
    static coroutine<void> seq_proc_range(Iterator begin, Iterator end) {
        for (Iterator it = begin; it != end; ++it)
            co_await (*it);
        co_return;
    }

    template <typename ValueType>
    static coroutine<void> seq_proc_vector(std::vector<ValueType> v) {
        for (auto &a: v)
            co_await std::move(a);
        co_return;
    }
    
    struct functor {
        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential(awaitables...)")]]
        constexpr auto operator()(Ts && ...ts) const {
            return by_value(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.by_value(awaitables...)")]]
        constexpr auto by_value(Ts && ...ts) const {
            return seq_proc_tuple<std::remove_cvref_t<Ts>...>(std::forward<Ts>(ts)... );
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.by_reference(awaitables...)")]]
        constexpr auto by_reference(Ts && ...ts) const {
            return seq_proc_tuple<Ts &&...>(std::forward<Ts>(ts)...);
        }

        template <typename ...Ts>
        [[nodiscard("expected usage: co_await sequential.copy_rvalues(awaitables...)")]]
        constexpr auto copy_rvalues(Ts && ...ts) const {
            return seq_proc_tuple<Ts...>(std::forward<Ts>(ts)...);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range(begin, end)")]]
        constexpr auto range(Iterator first, Iterator last) const {
            return range_copy(first, last);
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range_copy(begin, end)")]]
        constexpr auto range_copy(Iterator first, Iterator last) const {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            return seq_proc_vector(std::vector<value_type>(first, last));
        }

        template <typename Iterator>
        [[nodiscard("expected usage: co_await sequential.range_no_copy(begin, end)")]]
        constexpr auto range_no_copy(Iterator first, Iterator last) const {
            return seq_proc_range(first, last);
        }
    };
};

inline constexpr sequential_helper::functor sequential;
