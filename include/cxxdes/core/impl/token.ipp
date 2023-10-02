struct token_handler: memory::reference_counted_base<token_handler> {
    virtual void invoke(token *) {  }
    virtual ~token_handler() {  }
};

struct token: memory::reference_counted_base<token> {
    token(
        time_integral time,
        priority_type priority,
        coroutine_data_ptr coro_data,
        __attribute__ ((__unused__)) const char * what /* non-owning */= nullptr):
        time{time},
        priority{priority},
        coro_data{coro_data}
        #ifdef CXXDES_DEBUG_TOKEN
        , what{what}
        #endif /* CXXDES_DEBUG_TOKEN */
        {
        debug("token()");
    }

    // schedule time
    time_integral time = 0;

    // schedule priority
    priority_type priority = 0;

    // coroutine to continue
    coroutine_data_ptr coro_data = nullptr;

    // token handler can be modified only by all and any compositions
    memory::ptr<token_handler> handler = nullptr;

    // exception to be propagated
    std::exception_ptr eptr = nullptr;

    #ifdef CXXDES_DEBUG_TOKEN
    // a non-owning string describing the token
    const char *what = nullptr;
    #endif /* CXXDES_DEBUG_TOKEN */

    void attempt_access() {
        debug("attempt_access()");
    }

    ~token() {
        debug("~token()");
    }

private:
    void debug(__attribute__ ((__unused__)) const char * event) {
        #ifdef CXXDES_DEBUG_TOKEN
        fmt::println(
            "token@0x{}::{}: what = {}, ref_count = {}",
            (void *) this,
            event,
            what ? what : "(none)",
            ref_count()
        );
        #endif /* CXXDES_DEBUG_TOKEN */
    }
};
