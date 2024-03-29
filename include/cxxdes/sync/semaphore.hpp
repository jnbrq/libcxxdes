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

template <std::unsigned_integral U = std::size_t>
struct semaphore {
    semaphore(U value = 0, U max = std::numeric_limits<U>::max()):
        value_{value}, max_{max} {
        
    }

    U max() const {
        return max_;
    }

    U value() const {
        return value_;
    }

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
