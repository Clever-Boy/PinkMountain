#pragma once

// Assert.h — debug-break assertions (Lesson 1).
// PM_ENABLE_ASSERTS is defined for Debug builds in PinkMountain/CMakeLists.txt.
// Philosophy: asserts are for ENGINE bugs (broken invariants), not for bad
// user input. A failed assert logs, then breaks into the debugger so you see
// the exact call stack instead of a mystery crash three frames later.

#include "Pink/Core/Core.h"
#include "Pink/Core/Log.h"

#if defined(_MSC_VER)
    #define PM_DEBUG_BREAK() __debugbreak()
#else
    #define PM_DEBUG_BREAK() __builtin_trap()
#endif

#ifdef PM_ENABLE_ASSERTS
    // Always takes a message — an assert without a reason is a TODO, not a check.
    #define PM_CORE_ASSERT(x, msg)                                          \
        do {                                                                \
            if (!(x)) {                                                     \
                PM_CORE_ERROR("Assertion failed: {}", #x);                  \
                PM_CORE_ERROR("{}", msg);                                   \
                PM_DEBUG_BREAK();                                           \
            }                                                               \
        } while (0)
    #define PM_ASSERT(x, msg)                                               \
        do {                                                                \
            if (!(x)) {                                                     \
                PM_ERROR("Assertion failed: {}", #x);                       \
                PM_ERROR("{}", msg);                                        \
                PM_DEBUG_BREAK();                                           \
            }                                                               \
        } while (0)
#else
    #define PM_CORE_ASSERT(x, msg) do { (void)sizeof(x); } while (0)
    #define PM_ASSERT(x, msg)      do { (void)sizeof(x); } while (0)
#endif
