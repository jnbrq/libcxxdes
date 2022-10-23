template <typename T>
struct immediately_return {
    T return_value;

    bool await_ready() const noexcept { return true; }
    void await_suspend(coroutine_handle) const noexcept {  }
    T await_resume() const { return return_value; }
};

// Clang needs a deduction guide
template <typename A>
immediately_return(A &&a) -> immediately_return<std::remove_cvref_t<A>>;
