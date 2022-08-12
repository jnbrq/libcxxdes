#include <cxxdes/cxxdes.hpp>
#include <set>

#include "random_variable.hpp"

#include <fmt/core.h>

using namespace cxxdes;
using namespace cxxdes::time_ops;

// https://www.southampton.ac.uk/~sqc/EL336/CNL-7.pdf

struct aloha_config {
    std::size_t num_stations = 32; // units
    float lambda = 2.0f; // frames / second
    std::size_t packets_per_station = 10000; // frames
    float frame_time = 1.0f; // seconds / frame
};

struct aloha_result {
    float g;
    float s;
};

CXXDES_SIMULATION(aloha) {
    aloha(aloha_config const &cfg): cfg_{cfg} {
        env.time_unit(1_s);
        env.time_precision(1_ms);
    }

    const auto &config() const {
        return cfg_;
    }

    const auto &result() const {
        return result_;
    }

    process<> co_main() {
        std::vector<process<void>> ps;
        ps.reserve(cfg_.num_stations);
        for (std::size_t i = 0; i < cfg_.num_stations; ++i)
            ps.emplace_back(station(i));
        co_await all_of.range(ps.begin(), ps.end());

        result_.s = successful_transmissions_ / now_seconds();
        result_.g = cfg_.lambda * cfg_.frame_time;
    }

private:
    process<void> station(int id) {
        exponential_rv interarrival{rand_seed(), cfg_.lambda / cfg_.num_stations};

        for (std::size_t i = 0; i < cfg_.packets_per_station; ++i) {
            bool collided = false;

            active_transmissions_.insert(&collided);

            check_collisions();
            co_await timeout(cfg_.frame_time);

            active_transmissions_.erase(&collided);

            if (!collided) {
                successful_transmissions_++;
            }

            auto wait_time = interarrival();
            co_await timeout(wait_time);
        }
    }

    void check_collisions() {
        if (active_transmissions_.size() > 1) {
            for (auto collided: active_transmissions_)
                *collided = true;
        }
    }

    const aloha_config cfg_;
    aloha_result result_;

    std::size_t successful_transmissions_ = 0;
    std::set<bool *> active_transmissions_;
};

int main() {
    // experiment parameters
    const std::size_t num_steps = 50;
    const float begin = 0.1;
    const float end = 3.0;

    fmt::print("G [Erlang], S [Erlang]\n");

    for (std::size_t i = 0; i < num_steps; ++i) {
        const float lambda = begin + (end - begin) * i / (num_steps - 1);

        aloha experiment{
            aloha_config{
                .lambda = lambda, .packets_per_station = std::size_t(lambda * 1000)
            }
        };

        experiment.run();

        auto [g, s] = experiment.result();

        fmt::print("{}, {}\n", g, s);
    }

    return 0;
}
