#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

#include "random_variable.hpp"

using namespace cxxdes;

CXXDES_SIMULATION(producer_consumer_example) {
    producer_consumer_example(double lambda, double mu, std::size_t n_packets = 100000):
        lambda{ rand_seed() /* seed */, lambda /* lambda */},
        mu{ rand_seed() /* seed */, mu /* mu */ },
        n_packets{n_packets} {
    }

    double scale = 1.0e3; // 1000 simulation cycles is one second

    sync::queue<int> q;
    std::size_t n_packets;
    double total_latency = 0;

    exponential_rv lambda;
    exponential_rv mu;

    double avg_latency = 0.0;
    
    process<> producer() {
        for (std::size_t i = 0; i < n_packets; ++i) {
            co_await q.put(now());
            co_await timeout(lambda() * scale);
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
            
            co_await timeout(mu() * scale);

            total_latency += (now() - x) / scale;
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
