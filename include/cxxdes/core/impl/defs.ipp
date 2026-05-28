using coroutine_handle = std::coroutine_handle<>;

/** @brief Integral priority used to order events scheduled for the same time. */
using priority_type = std::intmax_t;

/** @brief Integral simulation timestamp measured in environment precision ticks. */
using time_integral = std::intmax_t;

/** @brief Floating-point type used for real-time reporting helpers. */
using real_type = double;

/** @brief Concrete time quantity type used by the core environment. */
using time = time_utils::time<time_integral>;
using time_utils::unitless_time;

/** @brief Type-erased time expression converted by an environment. */
using time_expr = time_utils::time_expr<time_integral>;

/** @brief Supported physical units for time quantities. */
using time_units = time_utils::time_unit_type;

/** @brief Operators and user-defined literals for time expressions. */
namespace time_ops = time_utils::ops;

/** @brief One second expressed as a core time quantity. */
constexpr auto one_second = time{1, time_units::seconds};

namespace priority_consts {

/** @brief Highest scheduling priority; lower numeric priorities run first. */
constexpr priority_type highest = std::numeric_limits<priority_type>::min();

/** @brief Sentinel that asks an awaitable to inherit the caller's priority. */
constexpr priority_type inherit = std::numeric_limits<priority_type>::max();

/** @brief Lowest explicit scheduling priority. */
constexpr priority_type lowest = inherit - 1;

/** @brief Default zero priority. */
constexpr priority_type zero = static_cast<priority_type>(0);

}
