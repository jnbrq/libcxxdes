/**
 * @file event.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Event class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_EVENT_HPP_INCLUDED
#define CXX_DES_EVENT_HPP_INCLUDED

#include <cstdint>
#include <concepts>
#include "coroutine.hpp"

namespace cxx_des {

using priority_type = std::intmax_t;
using time_type = std::uintmax_t;

struct event;

struct event_handler {
    virtual void invoke(event *evt);
    virtual ~event_handler() = default;
};

extern event_handler default_event_handler;

struct event {
    event(time_type time, priority_type priority, std::coroutine_handle<> coroutine_handle):
        time{time},
        priority{priority},
        coroutine_handle{coroutine_handle},
        handler{&default_event_handler},
        cleanup_handler{false} {  }

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
    std::coroutine_handle<> coroutine_handle = nullptr;

    /**
     * @brief Handles the event.
     * 
     */
    event_handler *handler = nullptr;

    /**
     * @brief Whether or not delete the event handler.
     * 
     */
    bool cleanup_handler = false;

    /**
     * @brief Processes the event, usually resumes the coroutine.
     * 
     */
    void process() {
        handler->invoke(this);
    }

    ~event() {
        if (cleanup_handler && handler)
            delete handler;
    }
};

inline void event_handler::invoke(event *evt) {
    if (evt->coroutine_handle && !evt->coroutine_handle.done())
        evt->coroutine_handle.resume();
}

}

#endif // CXX_DES_EVENT_HPP_INCLUDED
