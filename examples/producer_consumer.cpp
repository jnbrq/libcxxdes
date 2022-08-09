#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

#include "random_variable.hpp"

using namespace cxxdes;
using namespace cxxdes::time_ops;

CXXDES_SIMULATION(producer_consumer_example) {
    CXXDES_TIMESCALE(1_s, 1_ms)

    producer_consumer_example(double lambda, double mu, std::size_t n_packets = 100000):
        lambda{ rand_seed() /* seed */, lambda /* lambda */},
        mu{ rand_seed() /* seed */, mu /* mu */ },
        n_packets{n_packets} {
    }

    sync::queue<double> q;
    std::size_t n_packets;
    double total_latency = 0;

    exponential_rv lambda;
    exponential_rv mu;

    double avg_latency = 0.0;
    
    process<> producer() {
        for (std::size_t i = 0; i < n_packets; ++i) {
            co_await q.put(now_seconds());
            co_await timeout(lambda());
        }
    }

    process<> consumer() {
        std::size_t n = 0;

        while (true) {
            auto x = co_await q.pop();
            ++n;

            if (n == n_packets) {
                avg_latency = total_latency / n_packets;
                co_return ;
            }
            
            co_await timeout(mu());

            total_latency += (now_seconds() - x);
        }
    }

    process<> co_main() {
        co_await (producer() && consumer());
    }
};

int main() {
    double mu = 10.0;
    std::size_t n_steps = 100;

    fmt::print("{}, {}, {}, {}\n", "lambda", "mu", "rho", "avg_latency");
    for (std::size_t i = 0; i < n_steps; ++i) {
        double lambda = mu * i / (n_steps - 1);
        auto sim = producer_consumer_example{lambda, mu};
        sim.run();
        fmt::print("{}, {}, {}, {}\n", lambda, mu, lambda / mu, sim.avg_latency);
    }
    return 0;
}
