#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

struct clock_class {
    time_type period;
    std::string name;

    process operator()(environment *env) {
        while (true) {
            std::cout << name << " now = " << env->now() << std::endl;
            co_await timeout{period};
        }
    }
};

process clock(environment *env, time_type period, std::string name) {
    while (true) {
        std::cout << name << " now = " << env->now() << std::endl;
        co_await timeout{period};
    }
}

int main() {
    environment env;

    clock(&env, 2, "clock 1");
    clock(&env, 3, "clock 2");

    auto clock_instance = clock_class{7, "clock 3"};
    clock_instance(&env);

    while (env.step() && env.now() < 10) ;
}
