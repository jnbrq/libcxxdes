#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

process clock(environment *env, time_type period, std::string name) {
    while (true) {
        std::cout << name << " now = " << env->now() << std::endl;
        co_await timeout{period};
    }
}

int main() {
    environment env;

    clock(&env, 2, "clock 1");
    clock(&env, 3, "clock 2");

    while (env.step() && env.now() < 10) ;
}

#if 0

#include <coroutine>
#include <cstdint>
#include <vector>
#include <queue>

using time_type = std::uintmax_t;
using priority_type = std::intmax_t;

enum class event_type {
    none = 0,
    time,
    total
};

struct time_event: event {
    time_event(
        time_type t,
        std::coroutine_handle<> ch,
        priority_type priority = 1000):
        event{ch, priority},
        t_{t} {  }

    event_type type() const override {
        return event_type::time;
    }

    time_type time() const {
        return t_;
    }
private:
    time_type t_;
};

struct event_handler {
    virtual void append_event(event *evt) = 0;
    virtual event *pop_ready_event() = 0;

    virtual ~event_handler() = default;
};

struct default_event_handler: event_handler {
    void append_event(event *evt) override {
        events_.push(evt);
    }

    event *pop_ready_event() override {
        if (events_.empty())
            return nullptr;
        
        auto result = events_.front();
        events_.pop();
        return result;
    }
private:
    std::queue<event *> events_;
};

struct time_event_handler: event_handler {
    time_event_handler() {
    }

    void append_event(event *evt) override {
        events_.push((time_event *) evt);
    }

    event *pop_ready_event() override {
        if (events_.empty())
            return nullptr;
        
        auto result = events_.top();
        events_.pop();
        now_ = result->time();
        return result;
    }

    time_type now() const {
        return now_;
    }
private:
    time_type now_;

    /**
     * @brief A simple class for comparing two time_event's, for the priority queue.
     * 
     */
    struct time_comparator {
        bool operator()(time_event *evt_a, time_event *evt_b) const {
            return evt_a->time() < evt_b->time();
        }
    };

    std::priority_queue<time_event *, std::vector<time_event *>, time_comparator> events_;
};

struct simulator {
    void append_event(event *evt) {
        handlers_[(std::size_t) evt->type()]->append_event(evt);
    }

    bool step() {
        if (ready_events_.empty()) {
            // get all the ready events
            
        }
    }

    time_type now() const {
        time_event_handler_.now();
    }
private:
    struct priority_comparator {
        bool operator()(event *evt_a, event *evt_b) const {
            return evt_a->priority() < evt_b->priority();
        }
    };

    std::priority_queue<event *, std::vector<event *>, priority_comparator> ready_events_;

    default_event_handler default_event_handler_;
    time_event_handler time_event_handler_;

    event_handler *handlers_[(std::size_t) event_type::total] = {
        &default_event_handler_,
        &time_event_handler_
    };
};

int main() {

}

#endif
