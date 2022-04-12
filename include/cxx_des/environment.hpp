/**
 * @file environment.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Environment class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXX_DES_ENVIRONMENT_HPP_INCLUDED
#define CXX_DES_ENVIRONMENT_HPP_INCLUDED

#include "event.hpp"
#include <queue>

namespace cxx_des {

struct environment {

    environment() {  }

    time_type now() const {
        return now_;
    }

    void append_event(event *evt) {
        events_.push(evt);
    }

    bool step() {
        if (events_.empty())
            return false;
        
        auto evt = events_.top();
        events_.pop();

        now_ = std::max(evt->time, now_);
        evt->process();

        delete evt;

        return true;
    }

    ~environment() {
        while (!events_.empty()) {
            auto evt = events_.top();
            events_.pop();

            // TODO check if this is the correct way to manage the lifetime
            if (!evt->coroutine_handle.done()) {
                evt->coroutine_handle.destroy();
            }
            delete evt;
        }
    }

private:
    time_type now_ = 0;

    struct event_comp {
        bool operator()(event *evt_a, event *evt_b) const {
            return (evt_a->time > evt_b->time) ||
                (evt_a->time == evt_b->time && evt_a->priority > evt_b->priority);
        }
    };

    std::priority_queue<event *, std::vector<event *>, event_comp> events_;
};

}

#endif // CXX_DES_ENVIRONMENT_HPP_INCLUDED
