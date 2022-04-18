#include <cxxdes/cxxdes.hpp>
#include <iostream>

using namespace cxxdes;

CXXDES_SIMULATION(complicated_example) {
    sync::event evt;

    process<> p0() {
        co_await timeout(5);
    }

    process<> p1() {
        co_await timeout(5);
        co_await evt.wake();
    }

    process<> p2() {
        std::cout << "p2.a now " << env.now() << std::endl;
        co_await (evt.wait() || timeout(8));
        std::cout << "p2.b now " << env.now() << std::endl;
        co_await p0();
        std::cout << "p2.c now " << env.now() << std::endl;
        co_await sequential(timeout(5), timeout(5));
        std::cout << "p2.d now " << env.now() << std::endl;
    }

    // A very bad example
    process<> p3() {
        co_await sequential(timeout(5), timeout(10), p0());
        // do *not* use p0(env).start()!
        std::cout << "p3.a now " << env.now() << std::endl;
    }

    // Recursion example
    process<> p4(unsigned k) {
        if (k == 0)
            co_return ;
        
        co_await timeout(5);
        std::cout << "k = " << k << " now = " << env.now() << std::endl;
        co_await p4(k - 1);
    }

    process<> co_main() {
        std::cout << "Example 1: p1 and p2 working together. now = " << env.now() << std::endl;
        co_await all_of(p1(), p2());

        std::cout << "Example 2: p3 working alone. now = " << env.now() << std::endl;
        co_await p3();

        std::cout << "Example 3: p4 working alone. now = " << env.now() << std::endl;
        co_await p4(20);

        std::cout << "Example 4: same thing with control flow expressions. now = " << env.now() << std::endl;
        co_await ((p1() && p2()), p3(), p4(20));
    }
};

int main() {
    complicated_example{}.run();
    return 0;
}
