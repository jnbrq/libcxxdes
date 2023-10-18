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

template <typename T>
struct queue {
    queue(std::size_t max_size = 0 /* infinite */): max_size_{max_size} {
    }

    template <typename ...Args>
    [[nodiscard("expected usage: co_await queue.put(args...)")]]
    subroutine<> put(Args && ...args) {
        while (!can_put())
            co_await event_.wait();
        q_.emplace(std::forward<Args>(args)...);
        co_await event_.wake();
    }


    [[nodiscard("expected usage: co_await queue.pop()")]]
    subroutine<T> pop() {
        while (!can_pop())
            co_await event_.wait();
        auto v = std::move(q_.front());
        q_.pop();
        co_await event_.wake();
        co_return v;
    }

    std::size_t size() const noexcept {
        return q_.size();
    }

    std::size_t max_size() const noexcept {
        return max_size_;
    }

    bool bounded() const noexcept {
        return max_size() > 0;
    }

    bool can_put() const noexcept {
        return !bounded() || (max_size() > size());
    }

    bool can_pop() const noexcept {
        return size() > 0;
    }

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
