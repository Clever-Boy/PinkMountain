#pragma once

// SubSystem.h — the unit of engine modularity (Lesson 3).
// Every engine service (memory, windowing, renderer, audio...) is a SubSystem:
// initialized in priority order, updated every frame, shut down in REVERSE
// order. Reverse shutdown is the quiet hero — it guarantees a subsystem never
// outlives something it depends on (e.g. renderer never dies before the
// window whose context it draws into).

#include "Pink/Core/Core.h"

namespace Pink {

class PINK_API SubSystem
{
public:
    virtual ~SubSystem() = default;

    virtual void OnInit() {}
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
    virtual void OnFixedUpdate(float fixedDeltaTime) { (void)fixedDeltaTime; }
    virtual void OnShutdown() {}

    // Lower value = initialized earlier (and shut down later).
    // MemoryService returns -100 so allocators exist before everything.
    virtual int GetInitPriority() const { return 0; }
};

}
