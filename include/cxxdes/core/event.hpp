/**
 * @file event.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Event class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_EVENT_HPP_INCLUDED
#define CXXDES_CORE_EVENT_HPP_INCLUDED

#include <cstdint>
#include <concepts>
#include <cxxdes/core/coroutine.hpp>

namespace cxxdes {
namespace core {

using priority_type = std::intmax_t;
using time_type = std::uintmax_t;

struct event;

struct event_handler {
    /**
     * @brief Invoke *can* set evt->handler to null.
     * 
     * @param evt 
     */
    virtual void invoke(event *evt) = 0;
    virtual ~event_handler() = default;
};

/**
 * @brief There are two types of events:
 * 
 *   * Resuming events: event->handler == nullptr
 *     co_await A is valid only if A.on_suspend() returns a resuming event.
 *   * Non-resuming events: event->handler != nullptr
 *     Cannot be used for co_await's. They might trigger generation of other events.
 * 
 */
struct event {
    event(time_type time, priority_type priority, coro_handle coro):
        time{time},
        priority{priority},
        coro{coro},
        handler{nullptr} {  }

    /**
     * @brief Scheduled time of the event.
     * 
     */
    time_type time = 0;

    /**
     * @brief Priority of the event.
     * 
     */
    priority_type priority = 0;

    /**
     * @brief Corresponding coroutine.
     * 
     */
    coro_handle coro = nullptr;

    /**
     * @brief Handles the event.
     * 
     */
    event_handler *handler = nullptr;

    /**
     * @brief Processes the event, usually resumes the coroutine.
     * 
     */
    void process() {
        if (handler != nullptr)
            handler->invoke(this);
        else {
            if (coro && !coro.done())
                coro.resume();
        }
    }

    ~event() {
        if (handler)
            delete handler;
    }
};

} // namespace core
} // namespace cxxdes

#endif /* CXXDES_CORE_EVENT_HPP_INCLUDED */