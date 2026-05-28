/**
 * @file resource.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Resource synchronization primitive.
 * @date 2022-04-20
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_RESOURCE_HPP_INCLUDED
#define CXXDES_SYNC_RESOURCE_HPP_INCLUDED

#include <cxxdes/core/core.hpp>
#include <cxxdes/sync/semaphore.hpp>

namespace cxxdes {
namespace sync {

using core::coroutine;

/**
 * @brief Counted resource implemented on top of a semaphore.
 *
 * A resource models a fixed number of identical units. `acquire()` waits until
 * a unit is available and returns a move-only handle. The handle must be
 * released with `co_await handle.release()`; destroying the handle does not
 * release the resource.
 */
struct resource {
    /**
     * @brief Move-only token representing one acquired resource unit.
     *
     * A valid handle borrows the resource object and must not outlive it.
     */
    struct handle {
        /** @brief Constructs an invalid handle. */
        handle() = default;

        handle(handle const &) = delete;
        handle &operator=(handle const &) = delete;

        handle(handle &&other) {
            *this = std::move(other);
        }

        handle &operator=(handle &&other) {
            std::swap(x_, other.x_);
            return *this;
        }

        /** @brief Returns whether this handle currently owns a resource unit. */
        [[nodiscard]]
        bool valid() const noexcept {
            return x_ != nullptr;
        }
        
        /** @brief Equivalent to `valid()`. */
        [[nodiscard]]
        operator bool() const noexcept {
            return valid();
        }

        /**
         * @brief Releases the resource unit and wakes waiters.
         *
         * @throws std::runtime_error If the handle is invalid.
         */
        [[nodiscard("expected usage: co_await resource_handle.release()")]]
        subroutine<> release() {
            if (!valid())
                throw std::runtime_error("called release() on invalid resource handle");
            
            auto x = x_;
            x_ = nullptr;

            co_await x->s_.up();
        }

    private:
        friend struct resource;

        handle(resource *x): x_{x} {
        }

        resource *x_ = nullptr;
    };

    /** @brief Constructs a resource with @p count initially available units. */
    resource(std::size_t count): s_{count, count} {
    }

    /** @brief Waits for and acquires one resource unit. */
    [[nodiscard("expected usage: co_await resource.acquire()")]]
    subroutine<handle> acquire() {
        co_await s_.down();
        co_return handle(this);
    }
private:
    semaphore<> s_;
};

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_RESOURCE_HPP_INCLUDED */
