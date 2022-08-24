#define CXXDES_INTERRUPTABLE
#define CXXDES_DEBUG_CORE_PROCESS
#include <cxxdes/cxxdes.hpp>

using namespace cxxdes::core;

CXXDES_SIMULATION(test) {
    bool flag = false;

    process<> foo() {
        while (true) {
            co_await delay(10);
        }
    }

    process<> co_main() {
        co_await (foo() && foo());
    }
};

int main() {
    auto t = test{};
    t.run_for(100).stop();
    return 0;
}
