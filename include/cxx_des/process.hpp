/**
 * @file process.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Process class.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sonmez 2022
 * 
 */

#ifndef CXXDES_PROCESS_HPP_INCLUDED
#define CXXDES_PROCESS_HPP_INCLUDED

#include "event.hpp"
#include "environment.hpp"

namespace cxx_des {

struct process_class {
    environment *env = nullptr;
};

struct process final {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        environment *env;
        std::exception_ptr exception = nullptr;

        // function coroutines
        promise_type(environment *env, ...): env{env} {
            env->append_event(new event{ 0, -1000, handle_type::from_promise(*this) });
        }
        
        // class coroutines
        template <typename T>
        promise_type(T const &, environment *env, ...): promise_type{env} {  }

        process get_return_object() {
            return process(handle_type::from_promise(*this));
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception = std::current_exception(); }

        void return_void() {}
    };

    process(handle_type handle): handle_{handle} {
    }

    bool done() {
        return handle_.done();
    }

    bool operator()() {
        if (done())
            return true;
        
        handle_();

        if (auto e = handle_.promise().exception) {
            std::rethrow_exception(e);
        }

        return false;
    }

private:
    handle_type handle_;
};

template <typename T>
struct awaitable: T {
    
    // C++11 import base class constructors trick
    using T::T;

    bool await_ready() {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) {
        auto environment = process::handle_type::from_address(handle.address()).promise().env;
        T::on_suspend(environment, handle);
        return true;
    }

    auto await_resume() {
        return T::on_resume();
    }

};

}

#endif // CXXDES_PROCESS_HPP_INCLUDED
