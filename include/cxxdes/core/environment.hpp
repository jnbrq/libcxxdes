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

#include <cxxdes/core/token.hpp>
#include <queue>

namespace cxxdes {
namespace core {

struct environment {
    environment() {  }

    time_type now() const {
        return now_;
    }

    void schedule_token(token *tkn) {
        tokens_.push(tkn);
    }

    bool step() {
        if (tokens_.empty())
            return false;
        
        auto tkn = tokens_.top();
        tokens_.pop();

        now_ = std::max(tkn->time, now_);
        tkn->process();

        delete tkn;

        return true;
    }

    ~environment() {
        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            delete tkn;
        }
    }

private:
    time_type now_ = 0;

    struct token_comp {
        bool operator()(token *tkn_a, token *tkn_b) const {
            return (tkn_a->time > tkn_b->time) ||
                (tkn_a->time == tkn_b->time && tkn_a->priority > tkn_b->priority);
        }
    };

    std::priority_queue<token *, std::vector<token *>, token_comp> tokens_;
};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED */
