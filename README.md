# libcxxdes

A C++20 Discrete Event Simulation (DES) library.

## Debug Message Macros

All components of the library can be configured to have separate levels of warning/debug messages. This is possible thanks to extensive macro-based debugging infrastructure.

Newly added modules should use the following template to benefit from configurable debugging message levels:

```[cpp]

#ifndef NEW_COMPONENT_INCLUDED
#define NEW_COMPONENT_INCLUDED

#include <cxxdes/core/process.hpp>
// ... other cxxdes headers

// AFTER including ALL the cxxdes headers, add the following snippet
// BEFORE using the first debug message macro
#include <cxxdes/debug/helpers.hpp>
#ifdef CXXDES_DEBUG_NEW_COMPONENT
#   include <cxxdes/debug/begin.hpp>
#endif

// component code

#ifdef CXXDES_DEBUG_NEW_COMPONENT
#   include <cxxdes/debug/end.hpp>
#endif

#endif // NEW_COMPONENT_INCLUDED
```

To print the debug message from this module, simply define `CXXDES_DEBUG_NEW_COMPONENT` before including the header file (or, define that macro in the `CMakeLists.txt`):

```[cpp]

#define CXXDES_DEBUG_NEW_COMPONENT
#include <cxxdes/new_component.hpp>
```
