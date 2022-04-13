#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

struct clock_class {
    environment &env;
    time_type period;
    std::string name;

    process operator()() {
        while (true) {
            std::cout << name << " now = " << env.now() << std::endl;
            co_await timeout(period);
        }
    }
};

process clock(environment *env, time_type period, std::string name) {
    while (true) {
        std::cout << name << " now = " << env->now() << std::endl;
        co_await timeout(period);
    }
}

int main() {
    environment env;

    clock(&env, 2, "clock 1").start();
    clock(&env, 3, "clock 2").start();

    auto clock_instance = clock_class{env, 7, "clock 3"};
    clock_instance().start();

    while (env.step() && env.now() < 10) ;
}
