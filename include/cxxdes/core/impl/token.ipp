struct token_handler: memory::reference_counted_base<token_handler> {
    virtual void invoke(token *) {  }
    virtual ~token_handler() {  }
};

struct token: memory::reference_counted_base<token> {
    token(time_integral time, priority_type priority, coroutine_data_ptr coro_data):
        time{time},
        priority{priority},
        coro_data{coro_data} {  }

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
};
