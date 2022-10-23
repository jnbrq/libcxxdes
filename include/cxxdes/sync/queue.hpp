/**
 * @file queue.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Queue class.
 * @date 2022-04-17
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_QUEUE_HPP_INCLUDED
#define CXXDES_SYNC_QUEUE_HPP_INCLUDED

#include <queue>
#include <cxxdes/core/timeout.hpp>
#include <cxxdes/core/compositions.hpp>
#include <cxxdes/core/core.hpp>
#include <cxxdes/sync/event.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_SYNC_QUEUE
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace sync {

namespace detail {

using core::timeout;
using core::coroutine;
using core::operator&&;

template <typename T>
struct queue {
    queue(std::size_t max_size = 0 /* infinite */): max_size_{max_size} {
    }

    template <typename ...Args>
    [[nodiscard("expected usage: co_await queue.put(args...)")]]
    subroutine<> put(Args && ...args) {
        while (true) {
            if (max_size_ == 0 || q_.size() < max_size_)
                break ;
            co_await event_.wait();
        }
        q_.emplace(std::forward<Args>(args)...);
        co_await event_.wake();
    }


    [[nodiscard("expected usage: co_await queue.pop()")]]
    subroutine<T> pop() {
        while (true) {
            if (q_.size() > 0)
                break ;
            co_await event_.wait();
        }
        auto v = std::move(q_.front());
        q_.pop();
        co_await event_.wake();
        co_return v;
    }

    std::size_t size() const {
        return q_.size();
    }

    const std::queue<T> &underlying_queue() const {
        return q_;
    }
private:
    std::size_t max_size_;
    std::queue<T> q_;

    sync::event event_;
};

} /* namespace detail */

using detail::queue;

} /* namespace sync */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_SYNC_QUEUE
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_SYNC_QUEUE_HPP_INCLUDED */
