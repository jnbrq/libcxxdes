#ifndef CXX_DES_QUEUE_HPP_INCLUDED
#define CXX_DES_QUEUE_HPP_INCLUDED

#include <queue>
#include <cxxdes/sync/mutex.hpp>
#include <cxxdes/sync/event.hpp>

namespace cxxdes {
namespace queue {

namespace detail {

using core::process;

template <typename T>
struct queue {
    queue(std::size_t max_size = 0 /* infinite */) {

    }

    template <typename ...Args>
    process<> put(Args && ...args) {

    }

    process<> pop() {

    }
    
private:
    struct item {
    };

    std::queue<T> q_;
};

} // namespace detail

using detail::queue;

} // namespace queue
} // namespace cxxdes

#endif /* CXX_DES_QUEUE_HPP_INCLUDED */
