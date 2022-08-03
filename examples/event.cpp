#include <cxxdes/cxxdes.hpp>
#include <iostream>
#include <string>

using namespace cxxdes;

environment env;
sync::event evt;

process<> p1() {
    std::cout << "p1.a now " << env.now() << std::endl;
    co_await evt.wait(2, -100);
    std::cout << "p1.b now " << env.now() << std::endl;

    co_return ;
}

process<> p2() {
    std::cout << "p2.a now " << env.now() << std::endl;
    co_await timeout(5u);
    std::cout << "p2.b now " << env.now() << std::endl;
    co_await evt.wake(2);
    std::cout << "p2.c now " << env.now() << std::endl;

    co_return ;
}

process<> p3() {
    co_await evt.wait(8);
    std::cout << "p3.a now " << env.now() << std::endl;
    
    co_return ;
}

process<> p4() {
    std::cout << "p4.a now " << env.now() << std::endl;
    co_await timeout(20u);
    std::cout << "p4.b now " << env.now() << std::endl;
    co_await evt.wait(); // wakes up immediately
    std::cout << "p4.c now " << env.now() << std::endl;
    co_await evt.wait(2);
    std::cout << "p4.d now " << env.now() << std::endl;

    co_return ;
}

int main() {
    []() -> process<> {
        co_await all_of(p1(), p2(), p3(), p4());
    }().await_bind(&env);
    while (env.step()) ;

    return 0;
}
