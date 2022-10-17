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
#include <cxxdes/core/process.hpp>
#include <cxxdes/misc/utils.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_SYNC_MUTEX
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace sync {

using namespace cxxdes::core;

struct mutex {
    struct handle {
        handle() = default;

        handle(handle const &) = delete;
        handle &operator=(handle const &) = delete;

        handle(handle &&other) {
            *this = std::move(other);
        }

        handle &operator=(handle &&other) {
            std::swap(m_, other.m_);
            return *this;
        }

        [[nodiscard("expected usage: co_await handle.release()")]]
        process<> release() {
            if (!m_)
                throw std::runtime_error("called release() on invalid mutex handle");
            
            auto m = m_;
            m_ = nullptr;

            m->owned_ = false;
            co_await m->event_.wake();
        }
    private:
        friend struct mutex;

        handle(mutex *m): m_{m} {
        }

        mutex *m_ = nullptr;
    };

    [[nodiscard("expected usage: co_await mtx.acquire()")]]
    process<handle> acquire() {
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

#ifdef CXXDES_DEBUG_SYNC_MUTEX
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_SYNC_MUTEX_HPP_INCLUDED */
