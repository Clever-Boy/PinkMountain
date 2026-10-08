#pragma once

#include "Pink/Core/Core.h"

// Log.h — spdlog-based logging (Lesson 1).
// Two loggers: "PINK" for engine internals, "APP" for client (Sandbox)
// code. The split matters: when something breaks you can tell at a glance
// whether the engine or the game did it.

#include <spdlog/spdlog.h>

namespace Pink {

class PINK_API Log
{
public:
    static void Init();

    static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static Ref<spdlog::logger> s_CoreLogger;
    static Ref<spdlog::logger> s_ClientLogger;
};

}

// Engine-side macros
#define PM_CORE_TRACE(...)    ::Pink::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PM_CORE_INFO(...)     ::Pink::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PM_CORE_WARN(...)     ::Pink::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PM_CORE_ERROR(...)    ::Pink::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PM_CORE_CRITICAL(...) ::Pink::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client-side macros (Sandbox code)
#define PM_TRACE(...)         ::Pink::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PM_INFO(...)          ::Pink::Log::GetClientLogger()->info(__VA_ARGS__)
#define PM_WARN(...)          ::Pink::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PM_ERROR(...)         ::Pink::Log::GetClientLogger()->error(__VA_ARGS__)
#define PM_CRITICAL(...)      ::Pink::Log::GetClientLogger()->critical(__VA_ARGS__)
