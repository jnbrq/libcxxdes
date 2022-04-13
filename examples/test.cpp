#include <cxx_des/cxx_des.hpp>
#include <iostream>
#include <string>

using namespace cxx_des;

int main() {
    using namespace detail;
    any_of(timeout(1u), timeout(2u));
    // any_of_base<timeout, timeout, detail::event_fence::wait_type> t;

    return 0;
}
