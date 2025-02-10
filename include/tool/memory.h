#ifndef _TOOL_MEMORY_H
#define _TOOL_MEMORY_H

#include "basics.h"



//~ Definitions

#define TOOL_ARENA_COMMIT_SIZE 1024

#ifndef TOOL_NO_ALLOC
#define ALLOC(...) (Tool::Allocator(__VA_ARGS__))
#endif



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
    
    //~ Allocator
    
    typedef void* (*AllocatorFunction)(u64 size, void* data);
    
    struct MemoryAllocator
    {
        AllocatorFunction function;
        void* data;
    };
    
    
    
    //- Helper functions
    
    //~ Heap functionality
    
    void* ClassicAlloc(u64 size); 
    void* ClassicAlloc(u64 count, u64 size);
    template<typename T> inline T* ClassicAlloc() { return (T*)ClassicAlloc(sizeof(T)); }
    
    void ClassicDealloc(void* start);
    
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

    // Places new object onto the current arena frame
    // WARNING: objects created this way will NOT automatically destruct when popping the arena frame!
    template<typename T, class... Args> inline T* ArenaPlace(Arena* arena, Args&&... args) { return new ((T*)Tool::ArenaAlloc(arena, sizeof(T))) T((Args&&...)args...); }
    
    // Pushes a new arena frame.
    void ArenaPush(Arena* arena);
    
    // Pops the current arena frame. Throws an exception if currently in frame 0.
    void ArenaPop(Arena* arena);
    
    // De-initializes and frees memory arena.
    void ArenaDeInit(Arena* arena);
    
    //~ Allocators
    
    // Produces an allocator
    MemoryAllocator Allocator();             // Classic
    MemoryAllocator Allocator(Arena* arena); // Arena
    
    // Allocates space using the given allocator method
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size);
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size);
    template<typename T> inline T* AllocatorAlloc(MemoryAllocator allocator) { return (T*)AllocatorAlloc(allocator, sizeof(T)); }
    
}



#endif //_MEMORY_H
