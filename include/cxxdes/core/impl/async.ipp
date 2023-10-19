
struct async_functor {
    template <typename T>
    [[nodiscard("expected usage: co_await async(coroutine<T>)")]]
    constexpr auto operator()(coroutine<T> p) const {
        // since coroutine<T> is a reference-counted object with a flexible
        // lifetime, we can safely use coroutine<R> with async.
        // for other types of awaitables, they should be wrapped in
        // a coroutine to be used with async.
        struct async_awaitable {
            coroutine<T> p;

            void await_bind(environment *env, priority_type priority) {
                p.await_bind(env, priority);
                if (!p.await_ready())
                    // make sure that the completion token is setup correctly
                    // required to handle exceptions from async(...)
                    p.await_suspend(nullptr);
            }

            bool await_ready() {
                return true;
            }

            void await_suspend(coroutine_data_ptr) const noexcept {
            }

            token *await_token() const noexcept {
                return nullptr;
            }

            auto await_resume() const noexcept {
                return p;
            }

            void await_resume(no_return_value_tag) const noexcept {  }
        };

        return async_awaitable{p};
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async(awaitable)")]]
    constexpr auto operator()(A &&a) const {
        // We need to wrap the awaitable in a coroutine to support async.
        // There probably is not a good use case for this.
        return by_value(std::forward<A>(a));
    }
    
    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_value(awaitable)")]]
    constexpr auto by_value(A &&a) const {
        return (*this)(sequential.by_value(std::forward<A>(a)));
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_reference(awaitable)")]]
    constexpr auto by_reference(A &&a) const {
        return (*this)(sequential.by_reference(std::forward<A>(a)));
    }

    template <awaitable A>
    [[nodiscard("expected usage: co_await async.copy_rvalue(awaitable)")]]
    constexpr auto copy_rvalue(A &&a) const {
        return (*this)(sequential.copy_rvalues(std::forward<A>(a)));
    }
};

inline constexpr async_functor async;
