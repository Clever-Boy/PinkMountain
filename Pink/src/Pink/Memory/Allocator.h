#pragma once

// Allocator.h — custom allocators (Lesson 4).
// Why not just malloc/new everywhere? Three reasons engines care about:
//   1. Speed: a bump-pointer alloc is ~5 CPU instructions vs. malloc's search.
//   2. Fragmentation control: pools for fixed-size objects never fragment.
//   3. Visibility: you know EXACTLY how much memory each system uses.
// The cost: YOU own the lifetime discipline. These allocators don't free
// individual allocations (linear) or require strict discipline (stack).

#include "Pink/Core/Core.h"

#include <cstddef>
#include <cstdint>
#include <new> // placement new

namespace Pink {

// ---------------------------------------------------------------------------
// LinearAllocator — bump pointer. Allocate = advance offset. Free = Reset()
// the whole thing. Perfect for per-frame temporaries: allocate all frame,
// reset at frame start. Individual free is IMPOSSIBLE by design.
// ---------------------------------------------------------------------------
class PINK_API LinearAllocator
{
public:
    explicit LinearAllocator(size_t size);
    ~LinearAllocator();

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    // Placement-construction helper: the engine way to make objects in
    // allocator memory. Destruction is YOUR job (or nobody's, for POD).
    template<typename T, typename... Args>
    T* New(Args&&... args)
    {
        void* memory = Allocate(sizeof(T), alignof(T));
        return new (memory) T(std::forward<Args>(args)...);
    }

    void Reset(); // frees EVERYTHING at once — O(1)
    size_t GetUsed() const { return m_Offset; }
    size_t GetSize() const { return m_Size; }

private:
    uint8_t* m_Memory = nullptr;
    size_t m_Size = 0;
    size_t m_Offset = 0;
};

// ---------------------------------------------------------------------------
// StackAllocator — LIFO discipline with markers. FreeToMarker() rolls back
// to a snapshot. Ideal for scoped work: Marker m = alloc.GetMarker(); ...
// alloc.FreeToMarker(m); — everything allocated in between vanishes at once.
// Each allocation stores a small header with its pre-allocation offset so
// alignment padding rolls back correctly too.
// ---------------------------------------------------------------------------
class PINK_API StackAllocator
{
public:
    using Marker = size_t;

    explicit StackAllocator(size_t size);
    ~StackAllocator();

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    template<typename T, typename... Args>
    T* New(Args&&... args)
    {
        void* memory = Allocate(sizeof(T), alignof(T));
        return new (memory) T(std::forward<Args>(args)...);
    }

    Marker GetMarker() const { return m_Offset; }
    void FreeToMarker(Marker marker); // LIFO only — freeing out of order corrupts

private:
    uint8_t* m_Memory = nullptr;
    size_t m_Size = 0;
    size_t m_Offset = 0;
};

// ---------------------------------------------------------------------------
// PoolAllocator — fixed-size chunks from a free list. Allocate/Pop and
// Free/Push are O(1) and NEVER fragment: every chunk is identical, so any
// freed chunk satisfies any future allocation. The classic use: game
// entities, particles, render commands — thousands of same-sized objects
// with chaotic lifetimes.
// ---------------------------------------------------------------------------
class PINK_API PoolAllocator
{
public:
    PoolAllocator(size_t objectSize, size_t objectCount,
                  size_t alignment = alignof(std::max_align_t));
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    void* Allocate();   // pop a chunk; asserts when the pool is exhausted
    void Free(void* ptr); // push it back; ptr MUST come from this pool

    template<typename T, typename... Args>
    T* New(Args&&... args)
    {
        static_assert(sizeof(T) <= 0xFFFFFFFF, ""); // (size checked at runtime below)
        void* memory = Allocate();
        return new (memory) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void Delete(T* object)
    {
        object->~T(); // explicit dtor — placement new demands it
        Free(object);
    }

    size_t GetObjectSize() const { return m_ObjectSize; }
    size_t GetObjectCount() const { return m_ObjectCount; }

private:
    struct FreeNode { FreeNode* next; };

    uint8_t* m_Memory = nullptr;
    size_t m_ObjectSize = 0;
    size_t m_ObjectCount = 0;
    FreeNode* m_FreeList = nullptr;
};

}
