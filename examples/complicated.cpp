#include <cxx_des/cxx_des.hpp>
#include <iostream>

using namespace cxx_des;

event_fence fence;

process p0(environment *env) {
    co_await timeout(5);
}

process p1(environment *env) {
    co_await timeout(5);
    co_await fence.wake();
}

process p2(environment *env) {
    std::cout << "p2.a now " << env->now() << std::endl;
    co_await (fence.wait() || timeout(8));
    std::cout << "p2.b now " << env->now() << std::endl;
    co_await p0(env);
    std::cout << "p2.c now " << env->now() << std::endl;
    co_await sequential(timeout(5), timeout(5));
    std::cout << "p2.d now " << env->now() << std::endl;
}

// A very bad example
process p3(environment *env) {
    co_await sequential(timeout(5), timeout(10), p0(env));
    // do *not* use p0(env).start()!
    std::cout << "p3.a now " << env->now() << std::endl;
}

// Recursion example
process p4(environment *env, unsigned k) {
    if (k == 0)
        co_return ;
    
    co_await timeout(5);
    std::cout << "k = " << k << " now = " << env->now() << std::endl;
    co_await p4(env, k - 1);
}

process co_main(environment *env) {
    std::cout << "Example 1: p1 and p2 working together. now = " << env->now() << std::endl;
    co_await all_of(p1(env), p2(env));

    std::cout << "Example 2: p3 working alone. now = " << env->now() << std::endl;
    co_await p3(env);

    std::cout << "Example 3: p4 working alone. now = " << env->now() << std::endl;
    co_await p4(env, 20);

    std::cout << "Example 4: same thing with control flow expressions. now = " << env->now() << std::endl;
    fence.reset();
    co_await ((p1(env) && p2(env)), p3(env), p4(env, 20));
}

int main() {
    environment env;

    co_main(&env).latency(100).start();

    while (env.step()) ;

    return 0;
}
