#include <cxxdes/cxxdes.hpp>
#include <fmt/core.h>

using namespace cxxdes::core;

struct _get_env_tag {};

namespace cxxdes {
namespace core {

template <>
struct await_transform_extender<_get_env_tag> {
    template <typename ProcessData>
    auto await_transform(ProcessData *pdata) const noexcept {
        return immediately_return{pdata->env()};
    }
};

}
}

constexpr cxxdes::core::await_transform_extender<_get_env_tag> get_env;

CXXDES_SIMULATION(await_transform_extender_example) {
    process<> co_main() {
        co_await timeout(20);
        fmt::print("now = {}\n", (co_await get_env)->now());
    }
};

int main() {
    await_transform_extender_example{}.run();
    return 0;
}

