/**
 * @file begin.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Include this file at the beginning of the component to be debugged.
 * @date 2022-08-09
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#pragma push_macro("CXXDES_WARNING")
#pragma push_macro("CXXDES_DEBUG")

#undef CXXDES_WARNING
#undef CXXDES_DEBUG

#define CXXDES_WARNING(...) CXXDES_MESSAGE(std::cout, "WARNING", __VA_ARGS__)
#define CXXDES_DEBUG(...) CXXDES_MESSAGE(std::cout, "DEBUG", __VA_ARGS__)
