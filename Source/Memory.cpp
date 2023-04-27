
#include "Memory.h"
#include "Exception.h"

#include <Windows.h>



// TODO(crazy): Extend these to work on Linux and MacOS

namespace Tool
{
    //~ Heap functionality
    
    void* ClassicAlloc(u64 size)
    {
#ifdef TOOL_WINDOWS
        
        void* result = (void*)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
#ifndef TOOL_OPTIMIZED
        if (result == nullptr)
        {
            ExceptWindowsLast();
        }
#endif
        
        return result;
        
#endif // TOOL_WINDOWS
    }
    
    void* ClassicAlloc(u64 count, u64 size)
    {
        return ClassicAlloc(count * size);
    }
    
    void ClassicDealloc(void* start, u64 size)
    {
#ifdef TOOL_WINDOWS
        
        b32 result = VirtualFree(start, size, MEM_RELEASE);
        
#ifndef TOOL_OPTIMIZED
        if (!result)
        {
            ExceptWindowsLast();
        }
#endif
        
#endif // TOOL_WINDOWS
    }
    
    void ClassicDealloc(void* start, u64 count, u64 size)
    {
        ClassicDealloc(start, count * size);
    }
    
    void ClassicRealloc(void** target, u64 currentSize, u64 newSize)
    {
#ifdef TOOL_WINDOWS
        
        void* p = ClassicAlloc(newSize);
        Copy(p, *target, currentSize);
        ClassicDealloc(*target, currentSize);
        *target = p;
        
#endif // TOOL_WINDOWS
    }
    
    void ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size)
    {
        ClassicRealloc(target, currentCount * size, newCount * size);
    }
    
    
    
    //~ Memory region
    
    void RegionReserve(Region* region, u64 size)
    {
#ifdef TOOL_WINDOWS
        
        if (region->start != nullptr)
        {
            Except("Cannot reserve a Region which is already initialized."); 
        }
        
        region->start = (void*)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
        
#ifndef TOOL_OPTIMIZED
        if (region->start == nullptr)
        {
            ExceptWindowsLast();
        }
#endif
        
        region->reserved = size;
        region->committed = 0;
        
#endif // TOOL_WINDOWS
    }
    
    void RegionReserve(Region* region, u64 count, u64 size)
    {
        RegionReserve(region, count * size);
    }
    
    void RegionCommit(Region* region, u64 newSize)
    {
#ifdef TOOL_WINDOWS
        
        if (region->reserved < newSize)
        {
            Except("Cannot commit more memory to a Region than is reserved. (%ull > %ull)", newSize, region->reserved);
        }
        
        void* result = (void*)VirtualAlloc(region->start, newSize, MEM_COMMIT, PAGE_READWRITE);
        
#ifndef TOOL_OPTIMIZED
        if (region->start == nullptr)
        {
            ExceptWindowsLast();
        }
#endif
        
        region->committed = newSize;
        
#endif // TOOL_WINDOWS
    }
    
    void RegionCommit(Region* region, u64 newCount, u64 size)
    {
        RegionCommit(region, newCount * size);
    }
    
    void RegionRevert(Region* region, u64 newSize)
    {
#ifdef TOOL_WINDOWS
        
        if (newSize >= region->committed)
        {
            return;
        }
        
        b32 result = VirtualFree((u8*)region->start + newSize, region->committed - newSize, MEM_DECOMMIT); 
        
#ifndef TOOL_OPTIMIZED
        if (!result)
        {
            ExceptWindowsLast();
        }
#endif
        
        region->committed = newSize;
        
#endif // TOOL_WINDOWS
    }
    
    void RegionRevert(Region* region, u64 newCount, u64 size)
    {
        RegionRevert(region, newCount * size);
    }
    
    void RegionDealloc(Region* region)
    {
#ifdef TOOL_WINDOWS
        
        if (region->start == nullptr)
        {
            return;
        }
        
        b32 result = VirtualFree(region->start, region->reserved, MEM_RELEASE); 
        
#ifndef TOOL_OPTIMIZED
        if (!result)
        {
            ExceptWindowsLast();
        }
#endif
        
        region->start = nullptr;
        region->reserved = 0;
        region->committed = 0;
        
#endif // TOOL_WINDOWS
    }
    
    
    
    //~ Arena
    
    void ArenaInit(Arena* arena, u64 reservedSize)
    {
#ifdef TOOL_WINDOWS
        
        RegionReserve(&arena->region, reservedSize);
        RegionCommit(&arena->region, TOOL_ARENA_COMMIT_SIZE);
        arena->size = 0;
        
        arena->startCurrent = arena->region.start;
        arena->sizeCurrent = 0;
        
#endif // TOOL_WINDOWS
    }
    
    void* ArenaAlloc(Arena* arena, u64 size)
    {
#ifdef TOOL_WINDOWS
        
        u64 newSize = arena->size + size;
        
        if (newSize > arena->region.reserved)
        {
            Except("Cannot allocate more memory than is reserved in the Arena. (%ull + %ull > %ull)",
                   arena->size, size, arena->region.reserved);
        }
        
        if (newSize > arena->region.committed)
        {
            u64 newCommitSize = arena->region.committed * 2;
            newCommitSize = (newCommitSize > arena->region.reserved) ? arena->region.reserved : newCommitSize;
            
            RegionCommit(&arena->region, newSize);
        }
        
        void* result = (u8*)arena->startCurrent + arena->sizeCurrent;
        arena->sizeCurrent += size;
        arena->size = newSize;
        
        return result; 
        
#endif // TOOL_WINDOWS
    }
    
    void* ArenaAlloc(Arena* arena, u64 count, u64 size)
    {
        return ArenaAlloc(arena, count * size);
    }
    
    void ArenaPush(Arena* arena)
    {
#ifdef TOOL_WINDOWS
        
        ArenaFrame frame = { arena->startCurrent, arena->sizeCurrent };
        
        arena->startCurrent = (u8*)arena->startCurrent + arena->sizeCurrent;
        arena->sizeCurrent = 0;
        
        ArenaFrame* destination = (ArenaFrame*)ArenaAlloc(arena, sizeof(ArenaFrame));
        *destination = frame;
        
#endif // TOOL_WINDOWS
    }
    
    void ArenaPop(Arena* arena)
    {
#ifdef TOOL_WINDOWS
        
        ArenaFrame* frame = (ArenaFrame*)arena->startCurrent;
        
        arena->size -= arena->sizeCurrent;
        arena->startCurrent = frame->start;
        arena->sizeCurrent = frame->size;
        
#endif // TOOL_WINDOWS
    }
    
    void ArenaDenit(Arena* arena)
    {
#ifdef TOOL_WINDOWS
        
        RegionDealloc(&arena->region);
        
        arena->size = 0;
        arena->startCurrent = nullptr;
        arena->sizeCurrent = 0;
        
#endif // TOOL_WINDOWS
    }
}

