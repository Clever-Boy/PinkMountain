#include "Pink/Memory/Allocator.h"
#include "Pink/Core/Assert.h"

#include <cstdlib>

namespace Pink {

// --- LinearAllocator -------------------------------------------------------

LinearAllocator::LinearAllocator(size_t size)
    : m_Size(size)
{
    m_Memory = static_cast<uint8_t*>(std::malloc(m_Size));
    PM_CORE_ASSERT(m_Memory, "LinearAllocator: malloc failed");
}

LinearAllocator::~LinearAllocator()
{
    std::free(m_Memory);
}

void* LinearAllocator::Allocate(size_t size, size_t alignment)
{
    // Align the CURRENT offset up: padding bytes are simply skipped.
    // (alignment must be a power of two — true for every alignof result.)
    uintptr_t base = reinterpret_cast<uintptr_t>(m_Memory + m_Offset);
    uintptr_t aligned = (base + (alignment - 1)) & ~(alignment - 1);
    size_t newOffset = static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(m_Memory)) + size;

    PM_CORE_ASSERT(newOffset <= m_Size, "LinearAllocator out of memory — bump the size or Reset() more often");
    m_Offset = newOffset;
    return reinterpret_cast<void*>(aligned);
}

void LinearAllocator::Reset()
{
    m_Offset = 0;
}

// --- StackAllocator --------------------------------------------------------

StackAllocator::StackAllocator(size_t size)
    : m_Size(size)
{
    m_Memory = static_cast<uint8_t*>(std::malloc(m_Size));
    PM_CORE_ASSERT(m_Memory, "StackAllocator: malloc failed");
}

StackAllocator::~StackAllocator()
{
    std::free(m_Memory);
}

void* StackAllocator::Allocate(size_t size, size_t alignment)
{
    // The header records the offset BEFORE this allocation (including any
    // alignment padding), so FreeToMarker can always roll back exactly.
    struct Header { size_t prevOffset; };

    uintptr_t base = reinterpret_cast<uintptr_t>(m_Memory + m_Offset);
    uintptr_t aligned = (base + sizeof(Header) + (alignment - 1)) & ~(alignment - 1);

    Header* header = reinterpret_cast<Header*>(aligned - sizeof(Header));
    header->prevOffset = m_Offset;

    size_t newOffset = static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(m_Memory)) + size;
    PM_CORE_ASSERT(newOffset <= m_Size, "StackAllocator out of memory");
    m_Offset = newOffset;
    return reinterpret_cast<void*>(aligned);
}

void StackAllocator::FreeToMarker(Marker marker)
{
    // Rolling back is O(1): we abandon the memory, headers and all.
    // Markers from GetMarker() are always valid rollback points.
    PM_CORE_ASSERT(marker <= m_Offset, "StackAllocator: marker is newer than current top — LIFO violation");
    m_Offset = marker;
}

// --- PoolAllocator ---------------------------------------------------------

PoolAllocator::PoolAllocator(size_t objectSize, size_t objectCount, size_t alignment)
    : m_ObjectCount(objectCount)
{
    // Every chunk must hold a FreeNode and satisfy alignment: round up.
    m_ObjectSize = (objectSize + (alignment - 1)) & ~(alignment - 1);
    if (m_ObjectSize < sizeof(FreeNode))
        m_ObjectSize = sizeof(FreeNode);

    m_Memory = static_cast<uint8_t*>(std::malloc(m_ObjectSize * m_ObjectCount));
    PM_CORE_ASSERT(m_Memory, "PoolAllocator: malloc failed");

    // Thread the chunks into a free list.
    m_FreeList = reinterpret_cast<FreeNode*>(m_Memory);
    FreeNode* node = m_FreeList;
    for (size_t i = 0; i < m_ObjectCount - 1; ++i)
    {
        FreeNode* next = reinterpret_cast<FreeNode*>(
            reinterpret_cast<uint8_t*>(node) + m_ObjectSize);
        node->next = next;
        node = next;
    }
    node->next = nullptr;
}

PoolAllocator::~PoolAllocator()
{
    std::free(m_Memory);
}

void* PoolAllocator::Allocate()
{
    PM_CORE_ASSERT(m_FreeList, "PoolAllocator exhausted — increase objectCount");
    FreeNode* node = m_FreeList;
    m_FreeList = node->next;
    return node;
}

void PoolAllocator::Free(void* ptr)
{
    // Debug-only sanity: the pointer must land inside our block on a chunk
    // boundary. Catches "freed something that isn't mine" instantly.
    PM_CORE_ASSERT(ptr >= m_Memory && ptr < m_Memory + m_ObjectSize * m_ObjectCount,
        "PoolAllocator::Free on foreign pointer");
    PM_CORE_ASSERT((static_cast<uint8_t*>(ptr) - m_Memory) % m_ObjectSize == 0,
        "PoolAllocator::Free on misaligned chunk — not from this pool?");

    FreeNode* node = static_cast<FreeNode*>(ptr);
    node->next = m_FreeList;
    m_FreeList = node;
}

}
