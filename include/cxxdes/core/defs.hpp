/**
 * @file defs.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Basic type definitions.
 * @date 2022-08-24
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CORE_DEFS_HPP_INCLUDED
#define CXXDES_CORE_DEFS_HPP_INCLUDED

#include <cstddef>
#include <cxxdes/misc/time.hpp>

namespace cxxdes {
namespace core {

using priority_type = std::intmax_t;
using time_integral = std::intmax_t;
using real_type = double;

using time = time_utils::time<time_integral>;
using time_expr = time_utils::time_expr<time_integral>;
using time_units = time_utils::time_unit_type;

namespace time_ops = time_utils::ops;

constexpr auto one_second = time{1, time_units::seconds};

} /* namespace core */
} /* namespace cxxdes */

#endif /* CXXDES_CORE_DEFS_HPP_INCLUDED */
