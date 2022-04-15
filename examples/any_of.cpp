#include <cxx_des/cxx_des.hpp>
#include <iostream>

using namespace cxx_des;

static environment env;

process p1() {
    std::cout << "p1.a now " << env.now() << std::endl;
    co_await ((timeout(1000) && timeout(5)) || (timeout(100) && timeout(1)));
    std::cout << "p1.b now " << env.now() << std::endl;
    co_await all_of(timeout(10), timeout(20));
    std::cout << "p1.c now " << env.now() << std::endl;
    co_await any_of(timeout(10), timeout(20));
    std::cout << "p1.d now " << env.now() << std::endl;

    co_return ;
}

int main() {
    p1().start(env);

    while (env.step()) ;

    return 0;
}
