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

#include <queue>
#include <cxxdes/time.hpp>
#include <cxxdes/core/token.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_ENVIRONMENT
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

constexpr auto one_second = time<time_type>{1, time_unit_type::seconds};

struct environment {
    environment(time<time_type> const &unit = one_second, time<time_type> const &prec = one_second):
        now_{(time_type) 0}, unit_{unit}, prec_{prec} {
    }

    time_type now() const noexcept {
        return now_;
    }

    time<time_type> t() const noexcept {
        return { now() * prec_.t, prec_.u };
    }

    real_type now_seconds() const noexcept {
        return t().seconds<real_type>();
    }

    time<time_type> time_unit() const noexcept {
        return unit_;
    }

    time<time_type> time_precision() const noexcept {
        return prec_;
    }

    void schedule_token(token *tkn) {
        CXXDES_DEBUG_MEMBER_FUNCTION;
        CXXDES_DEBUG_VARIABLE(tkn);

        tokens_.push(tkn);
    }

    bool step() {
        CXXDES_DEBUG_MEMBER_FUNCTION;
        
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
        CXXDES_DEBUG_MEMBER_FUNCTION;
        
        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            delete tkn;
        }
    }

private:
    time_type now_ = 0;

    time<time_type> unit_;
    time<time_type> prec_;

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

#ifdef CXXDES_DEBUG_CORE_ENVIRONMENT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED */
