/**
 * @file cxx_des.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Universal include file.
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_CXXDES_HPP_INCLUDED
#define CXXDES_CXXDES_HPP_INCLUDED

// core
#include <cxxdes/core/event.hpp>
#include <cxxdes/core/timeout.hpp>
#include <cxxdes/core/environment.hpp>
#include <cxxdes/core/process.hpp>
#include <cxxdes/core/compositions.hpp>
#include <cxxdes/core/simulation.hpp>

// sync
#include <cxxdes/sync/event.hpp>
#include <cxxdes/sync/mutex.hpp>

namespace cxxdes {

using namespace core;

}

#endif /* CXXDES_CXXDES_HPP_INCLUDED */
