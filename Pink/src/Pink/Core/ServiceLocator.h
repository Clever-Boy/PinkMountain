#pragma once

// ServiceLocator.h — type-indexed service registry (Lesson 3).
// Answers: "how does code deep in the engine reach the MemoryService without
// a global variable or a 6-parameter constructor?" Provide() once at startup,
// Get<T>() anywhere after. The std::type_index key makes it typesafe: you
// can't retrieve a service as the wrong type.
//
// Trade-off vs. singletons: the locator is explicit about WHAT is global
// (registered services) and test code can Provide() mocks. Trade-off vs.
// pure dependency injection: less plumbing, but hidden dependencies —
// document which services each subsystem expects.

#include "Pink/Core/Core.h"
#include "Pink/Core/Assert.h"

#include <typeindex>
#include <unordered_map>

namespace Pink {

class PINK_API ServiceLocator
{
public:
    template<typename T>
    static void Provide(Ref<T> service)
    {
        s_Services[std::type_index(typeid(T))] = std::move(service);
    }

    template<typename T>
    static Ref<T> Get()
    {
        auto it = s_Services.find(std::type_index(typeid(T)));
        PM_CORE_ASSERT(it != s_Services.end(), "Service not registered — did you call Provide()?");
        return std::static_pointer_cast<T>(it->second);
    }

    template<typename T>
    static bool Has()
    {
        return s_Services.find(std::type_index(typeid(T))) != s_Services.end();
    }

    // Call at shutdown (after subsystems) to release everything.
    static void Clear() { s_Services.clear(); }

private:
    static std::unordered_map<std::type_index, Ref<void>> s_Services;
};

}
