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

#include <cxxdes/core/process.hpp>
#include <cxxdes/sync/semaphore.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_SYNC_RESOURCE
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace sync {

using core::process;

struct resource {
    struct handle {
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

        bool valid() const noexcept {
            return x_ != nullptr;
        }

        operator bool() const noexcept {
            return valid();
        }

        [[nodiscard("expected usage: co_await resource_handle.release()")]]
        process<> release() {
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

    resource(std::size_t count): s_{count, count} {
    }

    [[nodiscard("expected usage: co_await resource.acquire()")]]
    process<handle> acquire() {
        co_await s_.down();
        co_return handle(this);
    }
private:
    semaphore<> s_;
};

} /* namespace sync */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_SYNC_RESOURCE
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_SYNC_RESOURCE_HPP_INCLUDED */
