#include "Pink/Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Pink {

Ref<spdlog::logger> Log::s_CoreLogger;
Ref<spdlog::logger> Log::s_ClientLogger;

void Log::Init()
{
    // %^...%$ = color range, %T = HH:MM:SS, %n = logger name, %v = message.
    spdlog::set_pattern("%^[%T] %n: %v%$");

    s_CoreLogger = spdlog::stdout_color_mt("PINK");
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_mt("APP");
    s_ClientLogger->set_level(spdlog::level::trace);

    PM_CORE_INFO("Logging initialized");
}

}
