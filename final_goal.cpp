
// discrete event simulator

// A simple systolic array without double buffering
// Single memory banks, single computation unit
struct systolic_array_basic {
    environment env;
    bool done = false;

    process p() {
        while (!done) {
            auto instr = co_await queue_instr.pop();

            auto t_begin = env.now();

            // send a request to memory to load weights
            auto req_w = co_await mem_w.request(instr.addr_w, instr.k);
            auto req_a = co_await mem_x.request(instr.addr_x, instr.k);

            // wait until the memory is ready
            co_await (req_w && req_a); // control-flow expression are allowed

            // simulate the computation
            co_await timeout(delay_model(instr.k));

            auto t_end = env.now();
            auto time = t_end - t_begin;
        }
    }

    memory mem_w, mem_x;

    time_type delay_model(unsigned k) { /* microarchitecture-dependent computation model */ }

    void run() {
        p();
        while (env.step());
    }
};


// A double buffered systolic array
// Double the memory banks share the computation
class systolic_array_double_buffered {
    environment env;
    bool done = false;

    process p1() {
        while (!done) {
            auto instr = co_await queue_instr_1.pop();

            auto req_w = co_await mem_w_1.request(instr.addr_w, instr.k);
            auto req_a = co_await mem_x_1.request(instr.addr_a, instr.k);

            co_await (req_w && req_a);

            // exclusive access to the compute
            co_await mtx_computation.acquire();
            co_await timeout(delay_model(instr.k));
            co_await mtx_computation.release();
        }
    }

    process p2() { /* same */ }

    // rest...
};
