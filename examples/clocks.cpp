#include <cxxdes/cxxdes.hpp>
#include <iostream>
#include <string>

using namespace cxxdes;

environment env;

struct clock_class {
    time_type period;
    std::string name;

    process operator()() {
        while (true) {
            std::cout << name << " now = " << env.now() << std::endl;
            co_await timeout(period);
        }
    }
};

process clock(time_type period, std::string name) {
    while (true) {
        std::cout << name << " now = " << env.now() << std::endl;
        co_await timeout(period);
    }
}

int main() {
    clock(2, "clock 1").start(env);
    clock(3, "clock 2").start(env);

    auto clock_instance = clock_class{7, "clock 3"};
    clock_instance().start(env);

    while (env.step() && env.now() < 10) ;
}
