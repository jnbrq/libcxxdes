/**
 * @file semaphore.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Semaphore class.
 * @date 2022-04-20
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_SEMAPHORE_HPP_INCLUDED
#define CXXDES_SYNC_SEMAPHORE_HPP_INCLUDED

#include <limits>
#include <concepts>
#include <cxxdes/core/core.hpp>
#include <cxxdes/sync/event.hpp>

namespace cxxdes {
namespace sync {

using core::coroutine;

/**
 * @brief Counting semaphore for coordinating simulation processes.
 *
 * `down()` waits until the count is positive and then decrements it. `up()`
 * waits until the count is below the configured maximum and then increments it.
 * Both operations wake other waiters after changing the count.
 *
 * @tparam U Unsigned integer type used for the permit count.
 */
template <std::unsigned_integral U = std::size_t>
struct semaphore {
    /**
     * @brief Constructs a semaphore with an initial count and maximum count.
     *
     * The constructor does not validate that @p value is less than or equal to
     * @p max.
     */
    semaphore(U value = 0, U max = std::numeric_limits<U>::max()):
        value_{value}, max_{max} {
        
    }

    /** @brief Returns the maximum permit count. */
    U max() const {
        return max_;
    }

    /** @brief Returns the current permit count. */
    U value() const {
        return value_;
    }

    /** @brief Waits until the count can be incremented, then increments it. */
    [[nodiscard("expected usage: co_await semaphore.up()")]]
    subroutine<> up() {
        while (true) {
            if (value_ < max_)
                break ;
            co_await event_.wait();
        }
        ++value_;
        co_await event_.wake();
    }

    /** @brief Waits until a permit is available, then decrements the count. */
    [[nodiscard("expected usage: co_await semaphore.down()")]]
    subroutine<> down() {
        while (true) {
            if (value_ > 0)
                break ;
            co_await event_.wait();
        }
        --value_;
        co_await event_.wake();
    }

protected:
    U value_;
    U max_;

    sync::event event_;
};

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_SEMAPHORE_HPP_INCLUDED */
