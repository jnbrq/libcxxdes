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
#include <cxxdes/misc/reference_counted.hpp>
#include <cxxdes/core/defs.hpp>
#include <cxxdes/core/coroutine.hpp>
#include <cxxdes/misc/time.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_TOKEN
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

namespace priority_consts {

constexpr priority_type highest = std::numeric_limits<priority_type>::min();
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();
constexpr priority_type lowest = inherit - 1;
constexpr priority_type zero = static_cast<priority_type>(0);

}

struct token;

struct token_handler: memory::reference_counted_base<token_handler> {
    virtual void invoke(token *) {  }
    virtual ~token_handler() {  }
};

struct token {
    token(time_integral time, priority_type priority, coro_handle coro):
        time{time},
        priority{priority},
        coro{coro} {  }
        

    // schedule time
    time_integral time = 0;

    // schedule priority
    priority_type priority = 0;

    // coroutine to continue
    coro_handle coro = nullptr;

    // token handler can be modified only by all and any compositions
    memory::ptr<token_handler> handler = nullptr;

    void process() {
        CXXDES_DEBUG_MEMBER_FUNCTION;
        
        if (handler) {
            handler->invoke(this);
            return ;
        }

        if (coro && !coro.done())
            coro.resume();
    }

    ~token() {
    }
};

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_TOKEN
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_TOKEN_HPP_INCLUDED */
