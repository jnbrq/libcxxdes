struct exception_container {
    template <typename T>
    void assign(T &&t) {
        impl_.reset(new impl<std::remove_cvref_t<T>>(std::forward<T>(t)));
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return impl_.operator bool();
    }

    [[nodiscard]]
    operator bool() const noexcept {
        return valid();
    }

    void raise() {
        impl_->raise();
    }

private:
    struct underlying_type {
        virtual void raise() = 0;
        virtual ~underlying_type() = default;
    };

    template <typename T>
    struct impl: underlying_type {
        T t;

        template <typename U>
        impl(U &&u): t{std::forward<U>(u)} {
        }

        void raise() override {
            throw t;
        }

        virtual ~impl() = default;
    };

    std::unique_ptr<underlying_type> impl_ = nullptr;
};
