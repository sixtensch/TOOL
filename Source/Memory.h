#ifndef _MEMORY_H
#define _MEMORY_H

#include "Basics.h"



//~ Definitions

#define TOOL_ARENA_COMMIT_SIZE 1024



namespace Tool
{
    //- Struct definitions
    
    //~ Memory region
    
    struct Region
    {
        void* start = nullptr;
        u64 reserved;
        u64 committed;
    };
    
    //~ Arena
    
    struct ArenaFrame
    {
        void* start = nullptr;
        u64 size;
    };
    
    struct Arena
    {
        Region region;
        
        u64 size;
        
        void* startCurrent = nullptr;
        u64 sizeCurrent;
    };
    
    
    
    //- Helper functions
    
    //~ Heap functionality
    
    void* ClassicAlloc(u64 size); 
    void* ClassicAlloc(u64 count, u64 size);
    template<typename T> inline T* HeapAlloc() { return (T*)HeapAlloc(sizeof(T)); }
    
    void ClassicDealloc(void* start, u64 size);
    void ClassicDealloc(void* start, u64 count, u64 size);
    
    void ClassicRealloc(void** target, u64 currentSize, u64 newSize);
    void ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size);
    
    //~ Memory region
    
    // Initializes/reserves uninitialized region.
    void RegionReserve(Region* region, u64 size);
    void RegionReserve(Region* region, u64 count, u64 size);
    
    // Commits reserved memory pages. Cannot commit beyond reserved range.
    void RegionCommit(Region* region, u64 newSize);    
    void RegionCommit(Region* region, u64 newCount, u64 size);    
    
    // De-commits committed memory pages, reverting them to "reserved".
    void RegionRevert(Region* region, u64 newSize); 
    void RegionRevert(Region* region, u64 newCount, u64 size); 
    
    // Deallocates region, returning it to an uninitialized state.
    void RegionDealloc(Region* region);
    
    //~ Arena
    
    // Initializes and reserves memory arena. The allocated arena size may never exceed 'reservedSize'.
    void ArenaInit(Arena* arena, u64 reservedSize);
    
    // Allocates space within the current arena frame.
    void* ArenaAlloc(Arena* arena, u64 size);
    void* ArenaAlloc(Arena* arena, u64 count, u64 size);
    template<typename T> inline T* ArenaAlloc(Arena* arena) { return (T*)ArenaAlloc(arena, sizeof(T)); }
    
    // Pushes a new arena frame.
    void ArenaPush(Arena* arena);
    
    // Pops the current arena frame. Throws an exception if currently in frame 0.
    void ArenaPop(Arena* arena);
    
    // De-initializes and frees memory arena.
    void ArenaDeInit(Arena* arena);
}



#endif //_MEMORY_H
