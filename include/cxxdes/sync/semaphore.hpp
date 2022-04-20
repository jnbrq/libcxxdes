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

namespace cxxdes {
namespace sync {

namespace detail {

using core::timeout;
using core::process;
using core::operator&&;

struct semaphore {
    semaphore(std::size_t max = 0 /* no size limit */):
        max_{max}, value_{0} {
        
    }

    std::size_t max() const {
        return max_;
    }

    std::size_t value() const {
        return value_;
    }

    process<> up() {
        while (true) {
            co_await mutex_.acquire();
            if (max_ == 0 || value_ < max_)
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
    std::size_t max_;
    std::size_t value_;

    sync::event event_;
    sync::mutex mutex_;
};

} /* namespace detail */

using detail::semaphore;

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_SEMAPHORE_HPP_INCLUDED */
