
/**
 * @brief Starts an awaitable without blocking the current process.
 *
 * For `coroutine<T>`, `async` binds and starts the process, then immediately
 * resumes the caller with the coroutine handle. Await the returned handle later
 * if the caller needs the result or exception.
 */
struct async_functor {
    /** @brief Starts @p p and returns it without waiting for completion. */
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

    /** @brief Wraps a non-coroutine awaitable in a coroutine and starts it. */
    template <awaitable A>
    [[nodiscard("expected usage: co_await async(awaitable)")]]
    constexpr auto operator()(A &&a) const {
        // We need to wrap the awaitable in a coroutine to support async.
        // There probably is not a good use case for this.
        return by_value(std::forward<A>(a));
    }
    
    /** @brief Starts a copied/moved awaitable asynchronously. */
    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_value(awaitable)")]]
    constexpr auto by_value(A &&a) const {
        return (*this)(sequential.by_value(std::forward<A>(a)));
    }

    /** @brief Starts a referenced awaitable asynchronously; it must outlive the wrapper. */
    template <awaitable A>
    [[nodiscard("expected usage: co_await async.by_reference(awaitable)")]]
    constexpr auto by_reference(A &&a) const {
        return (*this)(sequential.by_reference(std::forward<A>(a)));
    }

    /** @brief Starts a copied rvalue awaitable asynchronously. */
    template <awaitable A>
    [[nodiscard("expected usage: co_await async.copy_rvalue(awaitable)")]]
    constexpr auto copy_rvalue(A &&a) const {
        return (*this)(sequential.copy_rvalues(std::forward<A>(a)));
    }
};

/** @brief Factory for asynchronous launch helpers. */
inline constexpr async_functor async;
