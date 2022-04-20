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

#include <cxxdes/core/compositions.hpp>
#include <cxxdes/core/process.hpp>
#include <cxxdes/sync/mutex.hpp>
#include <cxxdes/sync/event.hpp>

#include <limits>
#include <concepts>

namespace cxxdes {
namespace sync {

namespace detail {
namespace ns_semaphore {

using core::timeout;
using core::process;
using core::operator&&;

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

    process<> up() {
        while (true) {
            co_await mutex_.acquire();
            if (value_ < max_)
                break ;
            co_await (mutex_.release() && event_.wait());
        }
        ++value_;
        co_await (mutex_.release() && event_.wake());
    }

    process<> down() {
        while (true) {
            co_await mutex_.acquire();
            if (value_ > 0)
                break ;
            co_await (mutex_.release() && event_.wait());
        }
        --value_;
        co_await (mutex_.release() && event_.wake());
    }

protected:
    U value_;
    U max_;

    sync::event event_;
    sync::mutex mutex_;
};

} /* namespace ns_semaphore */
} /* namespace detail */

using detail::ns_semaphore::semaphore;

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_SEMAPHORE_HPP_INCLUDED */
