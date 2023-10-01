
template <awaitable A>
struct awaitable_wrapper {
    A a;
    coroutine_data_ptr phandle_this = nullptr;
    coroutine_data_ptr phandle_old = nullptr;
    
    bool await_ready() {
        return a.await_ready();
    }
    
    void await_suspend(coroutine_handle) {
        a.await_suspend(phandle_old);
    }

    auto await_resume() {
        if (phandle_this->interrupted())
            phandle_this->raise_interrupt_();
        return a.await_resume();
    }
};

struct this_coroutine {  };
struct this_environment {  };

template <typename T>
struct await_transform_extender;

namespace detail {

template <typename Derived>
struct await_ops_mixin {
    template <awaitable A>
    auto await_transform(
        A &&a,
        util::source_location const loc = util::source_location::current()) {
        auto result = awaitable_wrapper<std::remove_cvref_t<A>>{
            std::forward<A>(a),
            derived().coro_data.get(),
            derived().coro_data->env()->current_coroutine()
        };
        derived().coro_data->env()->loc_ = loc;
        result.a.await_bind(
            derived().coro_data->env(),
            derived().coro_data->priority());
        return result;
    }

    template <typename ReturnType>
    auto &&await_transform(subroutine<ReturnType> &&a);

    template <typename T>
    auto await_transform(
        await_transform_extender<T> const &a,
        util::source_location const loc = util::source_location::current()) {
        return a.await_transform(derived().coro_data, loc);
    }

    auto await_transform(this_coroutine) noexcept {
        return immediately_return{derived().coro_data};
    }

    auto await_transform(this_environment) noexcept {
        return immediately_return{derived().coro_data->env()};
    }
    
    template <awaitable A>
    auto yield_value(A &&a) {
        return await_transform(std::forward<A>(a));
    }

private:
    Derived &derived() noexcept {
        return *static_cast<Derived *>(this);
    }

    Derived const &derived() const noexcept {
        return *static_cast<Derived const *>(this);
    }
};

} /* namespace detail */
