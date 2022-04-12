#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

event_fence fence;

process p1(environment *env) {
    std::cout << "p1.a now " << env->now() << std::endl;
    co_await fence.wait(2, -100);
    std::cout << "p1.b now " << env->now() << std::endl;

    co_return ;
}

process p2(environment *env) {
    std::cout << "p2.a now " << env->now() << std::endl;
    co_await timeout(5u);
    std::cout << "p2.b now " << env->now() << std::endl;
    co_await fence.wake(2);
    std::cout << "p2.c now " << env->now() << std::endl;

    co_return ;
}

process p3(environment *env) {
    co_await fence.wait(8);
    std::cout << "p3.a now " << env->now() << std::endl;
    
    co_return ;
}

int main() {
    environment env;

    p1(&env);
    p2(&env);
    p3(&env);

    while (env.step()) ;

    return 0;
}
