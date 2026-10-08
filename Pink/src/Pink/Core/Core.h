#pragma once

// Core.h — the one header every engine header can rely on.
// Keep it LIGHT: no logging, no platform headers. Just language utilities.

// PINK_API is empty for the static-lib build. If the engine is ever
// built as a DLL/.so, define PM_DYNAMIC_LINK and flip this to the usual
// __declspec(dllexport/dllimport) / __attribute__((visibility)) dance.
#define PINK_API

#include <cstdint>
#include <memory>
#include <utility>

namespace Pink {

// Unique-ownership handle. Prefer Scope over raw new/delete everywhere.
template<typename T>
using Scope = std::unique_ptr<T>;

// Shared-ownership handle. Use sparingly — shared ownership is shared
// responsibility; most engine objects have one clear owner.
template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

}
