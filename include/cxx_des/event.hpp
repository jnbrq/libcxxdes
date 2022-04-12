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
#include "coroutine.hpp"

namespace cxx_des {

using priority_type = std::intmax_t;
using time_type = std::uintmax_t;

struct event {
    event(time_type time, priority_type priority, std::coroutine_handle<> coroutine_handle, bool skip = false):
        time{time}, priority{priority}, coroutine_handle{coroutine_handle}, skip{skip} {  }

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
     * @brief Flag to skip.
     * 
     */
    bool skip = false;

    /**
     * @brief Processes the event, usually resumes the coroutine.
     * 
     */
    virtual void process() {
        if (!skip && coroutine_handle && !coroutine_handle.done())
            coroutine_handle.resume();
    }

    virtual ~event() = default;
};

}

#endif // CXX_DES_EVENT_HPP_INCLUDED
