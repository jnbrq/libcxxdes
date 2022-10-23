using coroutine_handle = std::coroutine_handle<>;

using priority_type = std::intmax_t;
using time_integral = std::intmax_t;
using real_type = double;

using time = time_utils::time<time_integral>;
using time_utils::unitless_time;

using time_expr = time_utils::time_expr<time_integral>;
using time_units = time_utils::time_unit_type;

namespace time_ops = time_utils::ops;

constexpr auto one_second = time{1, time_units::seconds};

namespace priority_consts {

constexpr priority_type highest = std::numeric_limits<priority_type>::min();
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();
constexpr priority_type lowest = inherit - 1;
constexpr priority_type zero = static_cast<priority_type>(0);

}
