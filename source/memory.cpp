
#include "memory.h"
#include "exception.h"

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX
#include <sys/mman.h>
#include <unistd.h>
#endif



// TODO(crazy): Extend these to work on Linux and MacOS

namespace Tool
{
    //- Classic allocation
    
    //~ Classic allocation Windows implementation
    
#ifdef TOOL_WINDOWS
    
    void* ClassicAlloc(u64 size)
    {
        void* result = (void*)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        if (result == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return result;
    }
    
    void ClassicDealloc(void* start, u64 size)
    {
        b32 result = VirtualFree(start, size, MEM_RELEASE);
        
        if (!result)
        {
            ExceptWindowsLast();
        }
    }
    
#endif // TOOL_WINDOWS
    
    //~ Classic allocation Unix implementation
    
#ifdef TOOL_UNIX
    
    void* ClassicAlloc(u64 size)
    {
        void* result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (result == MAP_FAILED)
        {
            ExceptErrno();
        }
        
        return result;
    }
    
    void ClassicDealloc(void* start, u64 size)
    {
        i32 result = munmap(start, size);
        
        if (result < 0)
        {
            ExceptErrno();
        }
    }
    
#endif // TOOL_WINDOWS
    
    //~ Classic allocation general functions
    
    void ClassicRealloc(void** target, u64 currentSize, u64 newSize)
    {
        void* p = ClassicAlloc(newSize);
        Copy(p, *target, currentSize);
        ClassicDealloc(*target, currentSize);
        *target = p;
    }
    
    void* ClassicAlloc(u64 count, u64 size)
    {
        return ClassicAlloc(count * size);
    }
    
    void ClassicDealloc(void* start, u64 count, u64 size)
    {
        ClassicDealloc(start, count * size);
    }
    
    void ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size)
    {
        ClassicRealloc(target, currentCount * size, newCount * size);
    }
    
    
    
    //- Memory region
    
    //~ Memory region Windows implementation
    
#ifdef TOOL_WINDOWS
    
    void RegionReserve(Region* region, u64 size)
    {
        if (region->start != nullptr)
        {
            Except("Cannot reserve a Region which is already initialized."); 
        }
        
        region->start = (void*)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
        
        if (region->start == nullptr)
        {
            ExceptWindowsLast();
        }
        
        region->reserved = size;
        region->committed = 0;
    }
    
    void RegionCommit(Region* region, u64 newSize)
    {
        if (newSize >region->reserved)
        {
            Except("Cannot commit more memory to a Region than is reserved. (%ull > %ull)", newSize, region->reserved);
        }
        
        VirtualAlloc(region->start, newSize, MEM_COMMIT, PAGE_READWRITE);
        
        if (region->start == nullptr)
        {
            ExceptWindowsLast();
        }
        
        region->committed = newSize;
    }
    
    void RegionRevert(Region* region, u64 newSize)
    {
        if (newSize >= region->committed)
        {
            return;
        }
        
        b32 result = VirtualFree((u8*)region->start + newSize, region->committed - newSize, MEM_DECOMMIT); 
        
        if (!result)
        {
            ExceptWindowsLast();
        }
        
        region->committed = newSize;
    }
    
    void RegionDealloc(Region* region)
    {
        if (region->start == nullptr)
        {
            return;
        }
        
        b32 result = VirtualFree(region->start, 0, MEM_RELEASE); 
        
        if (!result)
        {
            ExceptWindowsLast();
        }
        
        region->start = nullptr;
        region->reserved = 0;
        region->committed = 0;
    }
    
#endif // TOOL_WINDOWS
    
    //~ Memory region Unix implementation
    
#ifdef TOOL_UNIX
    
    // On Unix, mmap does not actually allocate the memory. Physical pages will only be assigned when the memory is acted upon, such as by writing.
    
    // Reservation will mmap the entire region, and *protect* virtual pages beyond the boundary from being interacted with.
    
    static void UnixRegionProtect(void* memory, u64 accessible, u64 total)
    {
        static u64 pageSize = (u64)getpagesize();
        
        u64 accessibleAligned = pageSize * ((accessible - 1) / pageSize + 1);
        
        void* startAccessible = memory;
        void* startInaccessible = (void*)((char*)memory + accessibleAligned);
        
        i32 result = 0; 
        result |= mprotect(startAccessible, accessibleAligned, PROT_READ | PROT_WRITE);
        
        if (total > accessibleAligned)
        {
            result |= mprotect(startInaccessible, total - accessibleAligned, PROT_NONE);
        }
        
        if (result < 0)
        {
            ExceptErrno();
        }
    }
    
    void RegionReserve(Region* region, u64 size)
    {
        if (region->start != nullptr)
        {
            Except("Cannot reserve a Region which is already initialized."); 
        }
        
        // Initialize the memory with no access rights
        region->start = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (region->start == MAP_FAILED)
        {
            region->start = nullptr;
            ExceptErrno();
        }
        
        region->reserved = size;
        region->committed = 0;
    }
    
    void RegionCommit(Region* region, u64 newSize)
    {
        if (newSize > region->reserved)
        {
            Except("Cannot commit more memory to a Region than is reserved. (%ull > %ull)", newSize, region->reserved);
        }
        
        UnixRegionProtect(region->start, newSize, region->reserved);
        
        region->committed = newSize;
    }
    
    void RegionRevert(Region* region, u64 newSize)
    {
        if (newSize >= region->committed)
        {
            return;
        }
        
        UnixRegionProtect(region->start, newSize, region->reserved);
        
        region->committed = newSize;
    }
    
    void RegionDealloc(Region* region)
    {
        if (region->start == nullptr)
        {
            return;
        }
        
        i32 result = munmap(region->start, region->reserved);
        
        if (result < 0)
        {
            ExceptErrno();
        }
        
        region->start = nullptr;
        region->reserved = 0;
        region->committed = 0;
    }
    
#endif
    
    //~ Memory region general functions
    
    void RegionReserve(Region* region, u64 count, u64 size)
    {
        RegionReserve(region, count * size);
    }
    
    void RegionCommit(Region* region, u64 newCount, u64 size)
    {
        RegionCommit(region, newCount * size);
    }
    
    void RegionRevert(Region* region, u64 newCount, u64 size)
    {
        RegionRevert(region, newCount * size);
    }
    
    
    
    //- Arena
    
    //~ Arena general implementation
    
    void ArenaInit(Arena* arena, u64 reservedSize)
    {
        RegionReserve(&arena->region, reservedSize);
        RegionCommit(&arena->region, TOOL_ARENA_COMMIT_SIZE);
        arena->size = 0;
        
        arena->startCurrent = arena->region.start;
        arena->sizeCurrent = 0;
    }
    
    void* ArenaAlloc(Arena* arena, u64 size)
    {
        u64 newSize = arena->size + size;
        
        if (newSize > arena->region.reserved)
        {
            Except("Cannot allocate more memory than is reserved in the Arena. (%ull + %ull > %ull)",
                   arena->size, size, arena->region.reserved);
        }
        
        while (newSize > arena->region.committed)
        {
            u64 newCommitSize = arena->region.committed * 2;
            newCommitSize = (newCommitSize > arena->region.reserved) ? arena->region.reserved : newCommitSize;
            
            RegionCommit(&arena->region, newCommitSize);
        }
        
        void* result = (u8*)arena->startCurrent + arena->sizeCurrent;
        arena->sizeCurrent += size;
        arena->size = newSize;
        
        return result; 
    }
    
    void ArenaPush(Arena* arena)
    {
        ArenaFrame frame = { arena->startCurrent, arena->sizeCurrent };
        
        arena->startCurrent = (u8*)arena->startCurrent + arena->sizeCurrent;
        arena->sizeCurrent = 0;
        
        ArenaFrame* destination = (ArenaFrame*)ArenaAlloc(arena, sizeof(ArenaFrame));
        *destination = frame;
    }
    
    void ArenaPop(Arena* arena)
    {
        ArenaFrame* frame = (ArenaFrame*)arena->startCurrent;
        
        arena->size -= arena->sizeCurrent;
        arena->startCurrent = frame->start;
        arena->sizeCurrent = frame->size;
    }
    
    void ArenaDeInit(Arena* arena)
    {
        RegionDealloc(&arena->region);
        
        arena->size = 0;
        arena->startCurrent = nullptr;
        arena->sizeCurrent = 0;
    }
    
    void* ArenaAlloc(Arena* arena, u64 count, u64 size)
    {
        return ArenaAlloc(arena, count * size);
    }
    
    
    
    //- Allocators
    
    //~ Allocator triggers
    
    static void* AllocatorTriggerClassic(u64 size, void* data)
    {
        return ClassicAlloc(size);
    }
    
    static void* AllocatorTriggerArena(u64 size, void* data)
    {
        return ArenaAlloc((Arena*)data, size);
    }
    
    //~ Exposed allocator functions
    
    MemoryAllocator Allocator()             // Heap
    {
        return { &AllocatorTriggerClassic, nullptr };
    }
    
    MemoryAllocator Allocator(Arena* arena) // Arena
    {
        return { &AllocatorTriggerArena, (void*)arena };
    }
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size)
    {
        return allocator.function(size, allocator.data);
    }
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size)
    {
        return AllocatorAlloc(allocator, count * size);
    }
}

