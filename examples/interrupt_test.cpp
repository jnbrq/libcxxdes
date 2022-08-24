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

    process<> bar() {
        try {
            while (true)
                co_await delay(1000);
        }
        catch (interrupted_exception &ex) {
            flag = true;
            co_return ;
        }
    }

    process<> co_main() {
        while (true) {
            co_await delay(10);
        }
        // co_await foo();
    }
};

int main() {
    auto t = test{};
    t.run_for(100).stop();
    return 0;
}
