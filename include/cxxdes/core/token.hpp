/**
 * @file token.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Defines the token class.
 * @date 2022-08-03
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_TOKEN_HPP_INCLUDED
#define CXXDES_CORE_TOKEN_HPP_INCLUDED

#include <cstdint>
#include <limits>

#include <cxxdes/core/coroutine.hpp>

namespace cxxdes {
namespace core {


using priority_type = std::intmax_t;
using time_type = std::uintmax_t;

namespace priority_consts {

constexpr priority_type highest = std::numeric_limits<priority_type>::min();
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();
constexpr priority_type lowest = inherit - 1;
constexpr priority_type zero = static_cast<priority_type>(0);

}

struct token;

struct token_handler {
    virtual bool invoke(token *tkn) { return true; }
    virtual ~token_handler() {  }
};

struct token {
    token(time_type time, priority_type priority, coro_handle coro):
        time{time},
        priority{priority},
        coro{coro} {  }
        

    time_type time = 0;
    priority_type priority = 0;
    coro_handle coro = nullptr;

    token_handler *handler = nullptr;

    void process() {
        if (handler != nullptr)
            if (not handler->invoke(this))
                return ;
        if (coro && !coro.done())
            coro.resume();
    }

    ~token() {
        if (handler) delete handler;
    }
};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_TOKEN_HPP_INCLUDED */
