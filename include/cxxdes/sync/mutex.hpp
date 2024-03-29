/**
 * @file mutex.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Mutex.
 * @date 2022-04-13
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_MUTEX_HPP_INCLUDED
#define CXXDES_SYNC_MUTEX_HPP_INCLUDED

#include <queue>
#include <cxxdes/sync/event.hpp>
#include <cxxdes/core/core.hpp>
#include <cxxdes/misc/utils.hpp>

namespace cxxdes {
namespace sync {

using namespace cxxdes::core;

struct mutex {
    struct handle {
        handle() = delete;

        handle(handle const &) = delete;
        handle &operator=(handle const &) = delete;

        handle(handle &&other) {
            *this = std::move(other);
        }

        handle &operator=(handle &&other) {
            std::swap(x_, other.x_);
            return *this;
        }

        [[nodiscard]]
        bool valid() const noexcept {
            return x_ != nullptr;
        }

        [[nodiscard]]
        operator bool() const noexcept {
            return valid();
        }

        [[nodiscard("expected usage: co_await handle.release()")]]
        subroutine<> release() {
            if (!valid())
                throw std::runtime_error("called release() on invalid mutex handle");
            
            auto x = x_;
            x_ = nullptr;

            x->owned_ = false;
            co_await x->event_.wake();
        }
    private:
        friend struct mutex;

        handle(mutex *x): x_{x} {
        }

        mutex *x_ = nullptr;
    };

    [[nodiscard("expected usage: co_await mtx.acquire()")]]
    subroutine<handle> acquire() {
        while (true) {
            if (!owned_)
                break ;
            co_await event_.wait();
        }
        owned_ = true;
        co_return handle(this);
    }

    [[nodiscard]]
    bool is_acquired() const {
        return owned_;
    }

private:
    bool owned_ = false;
    sync::event event_;
};

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_MUTEX_HPP_INCLUDED */
