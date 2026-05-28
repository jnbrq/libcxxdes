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
#include <cxxdes/core/core.hpp>
#include <cxxdes/sync/event.hpp>

namespace cxxdes {
namespace sync {

namespace detail {

using core::timeout;
using core::coroutine;
using core::operator&&;

/**
 * @brief Coroutine-friendly FIFO queue with optional bounded capacity.
 *
 * `put()` suspends while a bounded queue is full. `pop()` suspends while the
 * queue is empty. A `max_size` of zero means the queue is unbounded.
 *
 * @tparam T Stored value type.
 */
template <typename T>
struct queue {
    /** @brief Constructs an empty queue; zero @p max_size means unbounded. */
    queue(std::size_t max_size = 0 /* infinite */): max_size_{max_size} {
    }

    /**
     * @brief Waits for capacity and constructs an item at the back of the queue.
     *
     * @param args Arguments forwarded to `std::queue<T>::emplace`.
     */
    template <typename ...Args>
    [[nodiscard("expected usage: co_await queue.put(args...)")]]
    subroutine<> put(Args && ...args) {
        while (!can_put())
            co_await event_.wait();
        q_.emplace(std::forward<Args>(args)...);
        co_await event_.wake();
    }


    /** @brief Waits for an item, removes the front item, and returns it. */
    [[nodiscard("expected usage: co_await queue.pop()")]]
    subroutine<T> pop() {
        while (!can_pop())
            co_await event_.wait();
        auto v = std::move(q_.front());
        q_.pop();
        co_await event_.wake();
        co_return v;
    }

    /** @brief Returns the number of currently stored items. */
    std::size_t size() const noexcept {
        return q_.size();
    }

    /** @brief Returns the configured maximum size, or zero when unbounded. */
    std::size_t max_size() const noexcept {
        return max_size_;
    }

    /** @brief Returns whether this queue has a finite capacity. */
    bool bounded() const noexcept {
        return max_size() > 0;
    }

    /** @brief Returns whether a producer can insert without blocking now. */
    bool can_put() const noexcept {
        return !bounded() || (max_size() > size());
    }

    /** @brief Returns whether a consumer can pop without blocking now. */
    bool can_pop() const noexcept {
        return size() > 0;
    }

    /** @brief Returns a const reference to the underlying `std::queue`. */
    const std::queue<T> &underlying_queue() const noexcept {
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

#endif /* CXXDES_SYNC_QUEUE_HPP_INCLUDED */
