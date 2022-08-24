/**
 * @file time.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Helper structs for time.
 * @date 2022-08-09
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef LIBCXXDES_INCLUDE_CXXDES_MISC_TIME_HPP_INCLUDED
#define LIBCXXDES_INCLUDE_CXXDES_MISC_TIME_HPP_INCLUDED

#include <cstdint>
#include <concepts>
#include <type_traits>
#include <limits>
#include <memory>

namespace cxxdes {
namespace time_utils {

enum class time_unit_type {
    seconds = 0,
    milliseconds,
    microseconds,
    nanoseconds,
    picoseconds,
    count
};

template <std::integral Integer = std::intmax_t>
struct time {
    struct node_tag {  };
    using integer_type = Integer;

    integer_type t = 0;
    time_unit_type u = time_unit_type::seconds;

    template <std::integral I>
    constexpr time(I tt, time_unit_type uu):
        t{(Integer) tt}, u{uu} {}
    
    template <typename I>
    constexpr time(time<I> const &other) noexcept {
        *this = other;
    }

    constexpr time(time const &other) noexcept {
        // -Wdeprecated-copy-with-user-provided-copy
        *this = other;
    }

    template <typename I>
    constexpr auto count(time<I> const &precision) const noexcept -> I {
        constexpr Integer conv [(int) time_unit_type::count] = {
            1, 1'000, 1'000'000, 1'000'000'000, 1'000'000'000'000
        };

        if (u < precision.u) {
            return conv[(int) precision.u - (int) u] * t / precision.t;
        }
        else {
            return (t / precision.t) / conv[(int) u - (int) precision.u];
        }
    }

    constexpr auto operator=(time const &other) noexcept -> auto & {
        if (&other != this) {
            t = other.t;
            u = other.u;
        }
        return *this;
    }

    template <typename I>
    constexpr auto operator=(time<I> const &other) noexcept -> auto & {
        auto new_t = other.t;
        auto new_u = (int) other.u;

        constexpr auto min = std::numeric_limits<Integer>::min();
        constexpr auto max = std::numeric_limits<Integer>::max();

        while ((new_t > max || new_t < min)) {
            new_t = new_t / 1000;
            new_u++;

            if (new_u == ((int) time_unit_type::count) - 1)
                break;
        }

        t = new_t;
        u = (time_unit_type) new_u;

        return *this;
    }

    template <typename RealType = double>
    constexpr auto seconds() const noexcept -> RealType {
        constexpr RealType conv [(int) time_unit_type::count] = {
            1, 1e-3, 1e-6, 1e-9, 1e-12
        };
        return (RealType) t * conv[(int) u];
    }
};

template <typename T>
concept node = requires {
    { std::declval<typename std::remove_cvref_t<T>::node_tag>() };
};

template <typename T>
concept scalar =
    std::floating_point<std::remove_cvref_t<T>> || std::integral<std::remove_cvref_t<T>>;

namespace ops {

template <typename A, typename B, typename Operation>
struct binary_node {
    struct node_tag {  };

    [[no_unique_address]] A a;
    [[no_unique_address]] B b;
    [[no_unique_address]] Operation op;

    template <typename I>
    constexpr auto count(time<I> const &precision) const noexcept -> I {
        return op(a, b, precision);
    }
};

template <typename A, typename Operation>
struct unary_node {
    struct node_tag {  };

    [[no_unique_address]] A a;
    [[no_unique_address]] Operation op;

    template <typename I>
    constexpr auto count(time<I> const &precision) const noexcept -> I {
        return op(a, precision);
    }
};

template <typename L, typename R, typename Operation>
constexpr auto make_node(L &&l, R &&r, Operation &&op) {
    using _L = std::remove_cvref_t<L>;
    using _R = std::remove_cvref_t<R>;
    using _Operation = std::remove_cvref_t<Operation>;
    return binary_node<_L, _R, _Operation>{
        std::forward<L>(l),
        std::forward<R>(r),
        std::forward<Operation>(op)
    };
}

template <typename L, typename Operation>
constexpr auto make_node(L &&l, Operation &&op) {
    using _L = std::remove_cvref_t<L>;
    using _Operation = std::remove_cvref_t<Operation>;
    return unary_node<_L, _Operation>{
        std::forward<L>(l),
        std::forward<Operation>(op)
    };
}

template <node A>
constexpr auto &&operator+(A &&a) {
    return std::forward<A>(a);
}

template <node A>
constexpr auto operator-(A &&a) {
    return make_node(
        std::forward<A>(a),
        [](auto const &a, auto const &p) {
            return -a.count(p);
        });
}

template <node A, node B>
constexpr auto operator+(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a.count(p) + b.count(p);
        });
}

template <node A, node B>
constexpr auto operator-(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a.count(p) - b.count(p);
        });
}

template <node A, scalar B>
constexpr auto operator*(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a.count(p) * b;
        });
}

template <scalar A, node B>
constexpr auto operator*(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a * b.count(p);
        });
}

template <node A, node B>
constexpr auto operator/(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a.count(p) / b.count(p);
        });
}

template <node A, scalar B>
constexpr auto operator/(A &&a, B &&b) {
    return make_node(
        std::forward<A>(a), std::forward<B>(b),
        [](auto const &a, auto const &b, auto const &p) {
            return a.count(p) / b;
        });
}

constexpr time<std::intmax_t> operator ""_s(unsigned long long x) {
    return time<std::intmax_t>{ (std::intmax_t) x, time_unit_type::seconds };
}

constexpr time<std::intmax_t> operator ""_ms(unsigned long long x) {
    return time<std::intmax_t>{ (std::intmax_t) x, time_unit_type::milliseconds };
}

constexpr time<std::intmax_t> operator ""_us(unsigned long long x) {
    return time<std::intmax_t>{ (std::intmax_t) x, time_unit_type::microseconds };
}

constexpr time<std::intmax_t> operator ""_ns(unsigned long long x) {
    return time<std::intmax_t>{ (std::intmax_t) x, time_unit_type::nanoseconds };
}

constexpr time<std::intmax_t> operator ""_ps(unsigned long long x) {
    return time<std::intmax_t>{ (std::intmax_t) x, time_unit_type::picoseconds };
}

} /* namespace ops */

template <std::integral Integer = std::intmax_t>
struct time_expr {
    struct node_tag {  };

    time_expr() = default;

    time_expr(time_expr const &other) {
        *this = other;
    }

    time_expr(time_expr &&other) {
        *this = std::move(other);
    }

    template <node T>
    requires (not std::is_same_v<std::remove_cvref_t<T>, time_expr>)
    time_expr(T &&t) {
        *this = std::forward<T>(t);
    }

    Integer count(time<Integer> const &p) const noexcept {
        if (not impl_) return 0;
        return impl_->count(p);
    }

    bool is_valid() const noexcept {
        return impl_;
    }

    template <node T>
    requires (not std::is_same_v<std::remove_cvref_t<T>, time_expr>)
    time_expr &operator=(T &&t) {
        struct impl: underlying_type {
            std::remove_cvref_t<T> value;

            impl(T &&t): value{std::move(t)} {
            }

            Integer count(time<Integer> const &p) const noexcept override {
                return value.count(p);
            }

            underlying_type *clone() const override {
                return (underlying_type *) (new impl{*this});
            }
        };
        impl_.reset(new impl(std::forward<T>(t)));
        return *this;
    }

    time_expr &operator=(time_expr const &other) {
        if (this != &other) {
            impl_.reset(other.impl_->clone());
        }
        return *this;
    }

    time_expr &operator=(time_expr &&other) {
        if (this != &other) {
            impl_ = std::move(other.impl_);
        }
        return *this;
    }
private:
    struct underlying_type {
        virtual Integer count(time<Integer> const &p) const noexcept = 0;
        virtual underlying_type *clone() const = 0;
        virtual ~underlying_type() = default;
    };

    std::unique_ptr<underlying_type> impl_;
};

} /* namespace time_utils */
} /* namespace cxxdes */

#endif /* LIBCXXDES_INCLUDE_CXXDES_MISC_TIME_HPP_INCLUDED */
