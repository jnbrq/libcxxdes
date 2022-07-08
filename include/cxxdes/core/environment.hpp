/**
 * @file environment.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Environment class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED
#define CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED

#include <cxxdes/core/event.hpp>
#include <queue>

namespace cxxdes {
namespace core {

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

// An alternative get_env implementation, slower, left as an example
#if 0

namespace detail {

struct get_env_type {
    event *on_suspend(promise_base *promise, coro_handle coro) {
        env_ = promise->env;
        auto evt = new event(env_->now(), 1000, coro);
        env_->append_event(evt);
        return evt;
    }

    environment *on_resume() {
        return env_;
    }

private:
    environment *env_ = nullptr;
};

} /* namespace detail */

inline auto get_env() {
    return detail::get_env_type{};
}

#endif

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED */
