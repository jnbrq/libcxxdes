#include <cxxdes/cxxdes.hpp>

#include <random>
#include <chrono>
#include <iostream>

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

using namespace cxxdes;

CXXDES_SIMULATION(producer_consumer_example) {
    double scale = 1.0e3; // 1000 simulation cycles is one second

    sync::queue<int> q;
    std::size_t total = 1000;
    double total_latency = 0;

    exponent_rv lambda{ 675 /* seed */, 4.0 /* lambda */ };
    exponent_rv mu{ 690 /* seed */, 4.0 /* mu */ };
    
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
            // std::cout << "t1 = " << x << " t2 = " << now() << std::endl;
            ++n;

            if (n == total) {
                std::cout << "Average latency = " << (total_latency / total) << " seconds.\n";
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
