#ifndef CXXDES_SYNC_QUEUE_HPP_INCLUDED
#define CXXDES_SYNC_QUEUE_HPP_INCLUDED

#include <queue>
#include <cxxdes/core/compositions.hpp>
#include <cxxdes/core/process.hpp>
#include <cxxdes/sync/mutex.hpp>
#include <cxxdes/sync/event.hpp>

namespace cxxdes {
namespace sync {

namespace detail {

using core::process;
using core::operator&&;

template <typename T>
struct queue {
    queue(std::size_t max_size = 0 /* infinite */): max_size_{max_size} {
    }

    template <typename ...Args>
    process<> put(Args && ...args) {
        while (true) {
            co_await mutex_.acquire();
            if (max_size_ == 0 || q_.size() < max_size_)
                break ;
            co_await (mutex_.release() && event_.wait());
        }
        q_.emplace(std::forward<Args>(args)...);
        co_await (mutex_.release() && event_.wake());
    }

    process<T> pop() {
        while (true) {
            co_await mutex_.acquire();
            if (q_.size() > 0)
                break ;
            co_await (mutex_.release() && event_.wait());
        }
        auto v = q_.front();
        q_.pop();
        co_await (mutex_.release() && event_.wake());
        co_return v;
    }
    
private:
    std::size_t max_size_;
    std::queue<T> q_;

    sync::event event_;
    sync::mutex mutex_;
};

} // namespace detail

using detail::queue;

} // namespace sync
} // namespace cxxdes

#endif /* CXXDES_SYNC_QUEUE_HPP_INCLUDED */
