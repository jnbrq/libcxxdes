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
#include <unordered_set>
#include <cxxdes/misc/time.hpp>
#include <cxxdes/core/token.hpp>

#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_CORE_ENVIRONMENT
#   include <cxxdes/debug/begin.hpp>
#endif

namespace cxxdes {
namespace core {

#ifdef CXXDES_INTERRUPTABLE

struct interrupted_exception: std::exception {
    const char *what() const noexcept override {
        return "interrupted.";
    }
};

struct coro_manager {
    void add_coro(coro_handle coro) {
        coros_.insert(coro);
    }

    void remove_coro(coro_handle coro) {
        if (!stopped())
            coros_.erase(coro);
    }

    bool stopped() const noexcept {
        return stopped_;
    }

    void stop() {
        stopped_ = true;
        for (auto coro: coros_)
            coro.resume();
        coros_.clear();
        stopped_ = false;
    }

private:
    struct hash: std::hash<void *> {
        size_t operator()(coro_handle x) const noexcept {
            return std::hash<void *>::operator()(x.address());
        }
    };

    std::unordered_set<coro_handle, hash> coros_;
    bool stopped_ = false;
};

#endif

struct environment {
    environment(time const &unit = one_second, time const &prec = one_second):
        now_{(time_integral) 0}, unit_{unit}, prec_{prec} {
    }

    time_integral now() const noexcept {
        return now_;
    }

    time t() const noexcept {
        return { now() * prec_.t, prec_.u };
    }

    real_type now_seconds() const noexcept {
        return t().seconds<real_type>();
    }

    void time_unit(time x) noexcept {
        if (used_)
            CXXDES_UNSAFE(__PRETTY_FUNCTION__, " called on a used environment!");
        
        unit_ = x;
    }

    time time_unit() const noexcept {
        return unit_;
    }

    void time_precision(time x) noexcept {
        if (used_)
            CXXDES_UNSAFE(__PRETTY_FUNCTION__, " called on a used environment!");
        
        prec_ = x;
    }

    time time_precision() const noexcept {
        return prec_;
    }

    template <cxxdes::time_utils::node Node>
    time_integral real_to_sim(Node const &n) const noexcept {
        return n.count(time_precision());
    }

    template <cxxdes::time_utils::scalar Scalar>
    time_integral real_to_sim(Scalar const &s) const noexcept {
        using time_ops::operator*;
        return (s * time_unit()).count(time_precision());
    }

    template <typename T>
    auto timeout(T &&t) const noexcept;

    void schedule_token(token *tkn) {
        CXXDES_DEBUG_MEMBER_FUNCTION;
        CXXDES_DEBUG_VARIABLE(tkn);

        used_ = true;
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

#ifdef CXXDES_INTERRUPTABLE
    coro_manager &get_coro_manager() noexcept {
        return coro_manager_;
    }

    coro_manager const &get_coro_manager() const noexcept {
        return coro_manager_;
    }

    void stop() {
        coro_manager_.stop();
    }
#endif

    ~environment() {
        CXXDES_DEBUG_MEMBER_FUNCTION;

#ifdef CXXDES_INTERRUPTABLE
        stop();
#endif
        while (!tokens_.empty()) {
            auto tkn = tokens_.top();
            tokens_.pop();
            delete tkn;
        }
    }

private:
    time_integral now_ = 0;
    bool used_ = false;

    time unit_;
    time prec_;

    struct token_comp {
        bool operator()(token *tkn_a, token *tkn_b) const {
            return (tkn_a->time > tkn_b->time) ||
                (tkn_a->time == tkn_b->time && tkn_a->priority > tkn_b->priority);
        }
    };

    std::priority_queue<token *, std::vector<token *>, token_comp> tokens_;

#ifdef CXXDES_INTERRUPTABLE
    coro_manager coro_manager_;
#endif
};

} /* namespace core */
} /* namespace cxxdes */

#ifdef CXXDES_DEBUG_CORE_ENVIRONMENT
#   include <cxxdes/debug/end.hpp>
#endif

#endif /* CXXDES_CORE_ENVIRONMENT_HPP_INCLUDED */
