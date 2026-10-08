#include "Pink/Memory/MemoryService.h"
#include "Pink/Core/Log.h"

namespace Pink {

MemoryService::MemoryService(size_t frameBytes, size_t poolObjectSize, size_t poolObjectCount)
    : m_FrameBytes(frameBytes)
    , m_PoolObjectSize(poolObjectSize)
    , m_PoolObjectCount(poolObjectCount)
{
}

void MemoryService::OnInit()
{
    m_FrameAllocator = CreateScope<LinearAllocator>(m_FrameBytes);
    m_Pool = CreateScope<PoolAllocator>(m_PoolObjectSize, m_PoolObjectCount);
    PM_CORE_INFO("MemoryService: frame allocator {} MB, pool {} x {} B",
        m_FrameBytes / (1024 * 1024), m_PoolObjectCount, m_PoolObjectSize);
}

void MemoryService::OnShutdown()
{
    // Allocators free their blocks in their dtors. Nothing outstanding may
    // still reference them after this point — reverse shutdown order (Lesson 3)
    // is what guarantees that.
    m_Pool.reset();
    m_FrameAllocator.reset();
    PM_CORE_INFO("MemoryService shut down");
}

}
