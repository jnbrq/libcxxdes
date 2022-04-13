#include <cxx_des/cxx_des.hpp>
#include <iostream>

using namespace cxx_des;

event_fence fence;

process p0(environment *env) {
    co_await timeout(5);

    co_return ;
}

process p1(environment *env) {
    co_await timeout(5);
    co_await fence.wake();

    co_return ;
}

process p2(environment *env) {
    std::cout << "p2.a now " << env->now() << std::endl;
    co_await (fence.wait() || timeout(8));
    std::cout << "p2.b now " << env->now() << std::endl;
    co_await p0(env);
    std::cout << "p2.c now " << env->now() << std::endl;
    // co_await sequential(p0(env), p0(env));
    // std::cout << "p2.d now " << env->now() << std::endl;
    co_return ;
}

int main() {
    environment env;

    p1(&env);
    p2(&env);

    while (env.step()) ;

    return 0;
}
