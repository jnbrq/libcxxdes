#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

process p1(environment *env) {
    std::cout << "p1.a now " << env->now() << std::endl;
    co_await ((timeout(1000) && timeout(5)) || (timeout(100) && timeout(1)));
    std::cout << "p1.b now " << env->now() << std::endl;

    co_return ;
}

int main() {
    environment env;

    p1(&env);

    while (env.step()) ;

    std::cout << "env.now() = " << env.now() << std::endl;

    return 0;
}
