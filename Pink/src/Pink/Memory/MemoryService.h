#pragma once

// MemoryService.h — owns the engine's allocators as a SubSystem (Lesson 4).
// Init priority -100: allocators exist before every other subsystem needs
// them. Other systems reach it via ServiceLocator<MemoryService> or a direct
// reference from Application::AddSubsystem.

#include "Pink/Core/Core.h"
#include "Pink/Core/SubSystem.h"
#include "Pink/Memory/Allocator.h"

namespace Pink {

class PINK_API MemoryService : public SubSystem
{
public:
    explicit MemoryService(size_t frameBytes = 16 * 1024 * 1024,
                           size_t poolObjectSize = 256,
                           size_t poolObjectCount = 1024);

    void OnInit() override;
    void OnShutdown() override;

    int GetInitPriority() const override { return -100; }

    // Per-frame scratch: allocate freely, Reset() at the start of each frame
    // (call from Application or a frame-graph later — NOT automatic yet).
    LinearAllocator& GetFrameAllocator() { return *m_FrameAllocator; }
    PoolAllocator& GetPool() { return *m_Pool; }

private:
    size_t m_FrameBytes;
    size_t m_PoolObjectSize;
    size_t m_PoolObjectCount;

    Scope<LinearAllocator> m_FrameAllocator;
    Scope<PoolAllocator> m_Pool;
};

}
