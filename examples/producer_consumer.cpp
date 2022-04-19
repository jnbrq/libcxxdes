#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>
#include <random>
#include <chrono>

template <typename Distribution, typename Engine = std::mt19937_64>
struct random_variable {
    template <typename Seed, typename ...Args>
    random_variable(Seed &&seed, Args && ...args):
        e_(std::forward<Seed>(seed)), d_(std::forward<Args>(args)...) {
    }

    auto operator()() {
        return d_(e_);
    }

private:
    Distribution d_;
    Engine e_;
};

using exponent_rv = random_variable<std::exponential_distribution<>>;

inline auto rand_seed() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

using namespace cxxdes;

CXXDES_SIMULATION(producer_consumer_example) {
    double scale = 1.0e3; // 1000 simulation cycles is one second

    sync::queue<int> q;
    std::size_t total = 100000;
    double total_latency = 0;

    exponent_rv lambda{ rand_seed() /* seed */, 4.0 /* lambda */ };
    exponent_rv mu{ rand_seed() /* seed */, 10.0 /* mu */ };
    
    process<> producer() {
        for (std::size_t i = 0; i < total; ++i) {
            co_await q.put(now());
            co_await timeout(lambda() * scale);
        }
    }

    process<> consumer() {
        std::size_t n = 0;

        while (true) {
            auto x = co_await q.pop();
            ++n;

            if (n == total) {
                fmt::print("Average latency = {} seconds.", total_latency / total);
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
    producer_consumer_example{}.run();
    return 0;
}
