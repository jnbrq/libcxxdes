#include <cxxdes/cxxdes.hpp>
#include <iostream>
#include <string>

using namespace cxxdes;
using namespace cxxdes::core;

environment env;
sync::event_fence fence;

process p1() {
    std::cout << "p1.a now " << env.now() << std::endl;
    co_await fence.wait(2, -100);
    std::cout << "p1.b now " << env.now() << std::endl;

    co_return ;
}

process p2() {
    std::cout << "p2.a now " << env.now() << std::endl;
    co_await timeout(5u);
    std::cout << "p2.b now " << env.now() << std::endl;
    co_await fence.wake(2);
    std::cout << "p2.c now " << env.now() << std::endl;

    co_return ;
}

process p3() {
    co_await fence.wait(8);
    std::cout << "p3.a now " << env.now() << std::endl;
    
    co_return ;
}

process p4() {
    std::cout << "p4.a now " << env.now() << std::endl;
    co_await timeout(20u);
    std::cout << "p4.b now " << env.now() << std::endl;
    co_await fence.wait(); // wakes up immediately
    std::cout << "p4.c now " << env.now() << std::endl;
    co_await fence.wait(2);
    std::cout << "p4.d now " << env.now() << std::endl;

    co_return ;
}

int main() {
    []() -> process {
        co_await all_of(p1(), p2(), p3(), p4());
    }().start(env);
    /*
    
    // Or, equivalently, you can write:

    p1().start(env);
    p2().start(env);
    p3().start(env);
    p4().start(env);

    */
    
    while (env.step()) ;

    return 0;
}
