#include <cstddef>
#include <stdio.h>

#if 0
static char mem[960000000];
char *next = mem;

void *operator new(size_t s) {
    if (s & 0x3)
        s = ((s >> 2) + 1) << 2; // align
    auto old = next;
    next += s;
    return old;
}

void operator delete(void *) noexcept {
}

void operator delete(void *, std::size_t) noexcept {
}
#endif

#include <cxxdes/cxxdes.hpp>
#include <stdio.h>

using namespace cxxdes;
using namespace cxxdes::core;
using namespace cxxdes::sync;

// std::pmr::monotonic_buffer_resource memres{100000000};

CXXDES_SIMULATION(ping_pong) {
    ping_pong() {
        // env.memres(&memres);
        // env.memres(std::pmr::new_delete_resource());
    }

    event a, b;

    process<> trigger() {
        co_await a.wake();
    }

    process<> party(
        event &a, event &b, const char *name, time_integral d) {
        while (true) {
            co_await a.wait();
            // printf("[%.0f] %s\n", now_seconds(), name);
            co_await delay(d);
            co_await b.wake();
        }
    }

    process<> co_main() {
        co_await all_of(
            party(a, b, "ping", 1).priority(0),
            party(b, a, "pong", 2).priority(0),
            trigger().priority(1)
        );
    }

private:
};

int main() {
    ping_pong{}.run_until(300000);
    return 0;
}
