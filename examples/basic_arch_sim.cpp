#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

#include <unordered_map>

using namespace cxxdes::core;

namespace arch {

using size_type = std::size_t;
using addr_type = std::size_t;

struct memory {
    memory(time_integral latency, size_type capacity, memory *next):
        latency_{latency}, capacity_{capacity}, next_{next} {
    }

    coroutine<> load(addr_type addr) {
        time_integral timestamp = (co_await this_environment())->now();

        // co_await timeout(extra_latency_); // additional latency to be added here
        _Co_with(mtx_) {
            if (next_ && !blocks_.count(addr)) {
                if (blocks_.size() == capacity_) {
                    // evict the oldest block (a better way to keep track of the oldest accessed element?)
                    auto it = std::min_element(
                        blocks_.begin(),
                        blocks_.end(),
                        [](auto const &a, auto const &b) {
                            return a.second.last_access < b.second.last_access;
                        });

                    if (it->second.dirty) {
                        co_await next_->store(it->second.addr);
                    }

                    blocks_.erase(it);
                }

                // wait until loaded
                co_await next_->load(addr);
            }
            co_await timeout(latency_);
            blocks_[addr].last_access = timestamp;
        };
    }

    coroutine<> store(addr_type addr) {
        // time_integral timestamp = (co_await this_coroutine::get_environment())->now();

        // co_await timeout(extra_latency_); // additional latency to be added here
        _Co_with(mtx_) {
            // given the fact that this is an inclusive memory hierarchy,
            // we are guaranteed to have this block in this memory
            co_await timeout(latency_);
            blocks_[addr].dirty = true;
        };
    }

private:
    time_integral latency_;
    size_type capacity_;
    memory *next_;
    cxxdes::sync::mutex mtx_; // for the shared bandwidth

    struct block {
        addr_type addr = 0;
        bool dirty = false;
        time_integral last_access = 0;
    };

    std::unordered_map<addr_type, block> blocks_;
};

}

CXXDES_SIMULATION(memory_test) {
    using simulation::simulation;
    
    arch::memory mem1{10 /* latency */, 1024 /* capacity */, nullptr /* next level */};
    arch::memory mem2{1, 16, &mem1};

    coroutine<> co_main() {
        for (arch::addr_type i = 0; i < 32; ++i) {
            auto j = i % 16;
            auto t1 = now();
            co_await mem2.load(j);
            auto t2 = now();
            fmt::print("time to load block {} = {}\n", j, t2 - t1);
        }

        for (arch::addr_type i = 0; i < 32; ++i) {
            auto j = i;
            auto t1 = now();
            co_await mem2.load(j);
            auto t2 = now();
            fmt::print("time to load block {} = {}\n", j, t2 - t1);
        }

        for (arch::addr_type i = 0; i < 16; ++i) {
            auto j = i;
            auto t1 = now();
            co_await mem2.load(j);
            auto t2 = now();
            fmt::print("time to load block {} = {}\n", j, t2 - t1);
        }

        co_return ;
    }
};

int main() {
    memory_test{}.run();
    return 0;
}
