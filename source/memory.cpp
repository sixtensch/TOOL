#include "memory.h"
#include "exception.h"
#include "mathematics.h"

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX
#include <sys/mman.h>
#include <unistd.h>
#endif



//- Static helper functions

//~ Windows static helper functions

#ifdef TOOL_WINDOWS

// Get DWORD parts
static DWORD GetLowDWORD(u64 whole) { return (DWORD)(whole & 0xffffffff); }
static DWORD GetHighDWORD(u64 whole) { return (DWORD)(whole >> 0x20); }

// Round a given size up to the nearest multiple of the granularity, which itself is a power of 2
static u64 RoundToGranularity(u64 size, u64 granularity)
{
    return (size + granularity - 1) & ~(granularity - 1);
}

#endif



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
    
    void ClassicDealloc(void* start)
    {
        b32 result = VirtualFree(start, 0, MEM_RELEASE);
        
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
        ClassicDealloc(*target);
        *target = p;
    }
    
    void* ClassicAlloc(u64 count, u64 size)
    {
        return ClassicAlloc(count * size);
    }
    
    void ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size)
    {
        ClassicRealloc(target, currentCount * size, newCount * size);
    }
    
    
    
    //- Memory region
    
    //~ Memory region Windows implementation
    
#ifdef TOOL_WINDOWS
    
    void RegionReserve(MemoryRegion* region, u64 size)
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
    
    void RegionCommit(MemoryRegion* region, u64 newSize)
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
    
    void RegionRevert(MemoryRegion* region, u64 newSize)
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
    
    void RegionDealloc(MemoryRegion* region)
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
    
    void RegionReserve(MemoryRegion* region, u64 size)
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
    
    void RegionCommit(MemoryRegion* region, u64 newSize)
    {
        if (newSize > region->reserved)
        {
            Except("Cannot commit more memory to a Region than is reserved. (%ull > %ull)", newSize, region->reserved);
        }
        
        UnixRegionProtect(region->start, newSize, region->reserved);
        
        region->committed = newSize;
    }
    
    void RegionRevert(MemoryRegion* region, u64 newSize)
    {
        if (newSize >= region->committed)
        {
            return;
        }
        
        UnixRegionProtect(region->start, newSize, region->reserved);
        
        region->committed = newSize;
    }
    
    void RegionDealloc(MemoryRegion* region)
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
    
    void RegionReserve(MemoryRegion* region, u64 count, u64 size)
    {
        RegionReserve(region, count * size);
    }
    
    void RegionCommit(MemoryRegion* region, u64 newCount, u64 size)
    {
        RegionCommit(region, newCount * size);
    }
    
    void RegionRevert(MemoryRegion* region, u64 newCount, u64 size)
    {
        RegionRevert(region, newCount * size);
    }
    
    
    
    //- Memory loop
    
    //~ Memory loop Windows implementation
    
#ifdef TOOL_WINDOWS
    
    //~ Definitions and function types for runtime loading
    
#ifndef MEM_PRESERVE_PLACEHOLDER
    struct MEM_EXTENDED_PARAMETER;
#define MEM_PRESERVE_PLACEHOLDER 0x2
#define MEM_REPLACE_PLACEHOLDER 0x4000
#define MEM_RESERVE_PLACEHOLDER 0x40000
#define MemExtendedParameterAddressRequirements 0x1
#endif
    
    // Function type definitions mirroring those in memoryapi.h (on newer SDKs)
    
    typedef PVOID (*VirtualAlloc2Function)(HANDLE Process,
                                           PVOID BaseAddress,
                                           SIZE_T Size,
                                           ULONG AllocationType,
                                           ULONG PageProtection,
                                           MEM_EXTENDED_PARAMETER* ExtendedParameters,
                                           ULONG ParameterCount);
    
    typedef PVOID (*MapViewOfFile3Function)(HANDLE FileMapping,
                                            HANDLE Process,
                                            PVOID BaseAddress,
                                            ULONG64 Offset,
                                            SIZE_T ViewSize,
                                            ULONG AllocationType,
                                            ULONG PageProtection,
                                            MEM_EXTENDED_PARAMETER* ExtendedParameters,
                                            ULONG ParameterCount);
    
    // Initializes/reserves uninitialized memory loop.
    void LoopAlloc(MemoryLoop* loop, u64 minCommittedSize, u64 minMirroredSize)
    {
        if (loop->start != nullptr)
        {
		    Except("Cannot initialize a Memory Loop which is already initialized.");
        }
        
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        
        u64 granularity = (u64)systemInfo.dwAllocationGranularity; // Most likely 64k
        u64 committedSize = RoundToGranularity(minCommittedSize, granularity);
        u64 mirroredSize = RoundToGranularity(minMirroredSize, granularity);
        u64 totalSize = committedSize + mirroredSize;
        
        // This does not represent an actual file, but rather an anonymous memory allocation backed by the system paging file.
        HANDLE fileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, 
                                                PAGE_READWRITE, 
                                                GetHighDWORD(committedSize), 
                                                GetLowDWORD(committedSize),
                                                nullptr);
        
        if (fileMapping == NULL || fileMapping == INVALID_HANDLE_VALUE)
        {
            ExceptWindowsLast();
        }
        
        // Try to load the modern Windows runtime functions from the kernelbase system dll. Performance shouldn't be a big issue.
        HMODULE kernel = LoadLibraryA("kernelbase.dll");
        
        if (kernel == NULL)
        {
            ExceptWindowsLast();
        }
        
        VirtualAlloc2Function virtualAlloc2 = (VirtualAlloc2Function)(void*)GetProcAddress(kernel, "VirtualAlloc2");
        MapViewOfFile3Function mapViewOfFile3 = (MapViewOfFile3Function)(void*)GetProcAddress(kernel, "MapViewOfFile3");
        
        char* start = nullptr;
        if (virtualAlloc2 && mapViewOfFile3) // If they exist, then use them
        {
            // First, reserve the whole range, both committed and mirrored, in the virtual address space
            start = (char*)virtualAlloc2(0, 0,
                                         totalSize,
                                         MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,	// Reserve the space, and designate it as a placeholder
                                         PAGE_NOACCESS,							// The pages cannot be accessed in this state
                                         0, 0);
            
            if (start == NULL)
            {
                ExceptWindowsLast();
            }
            
            // Then, remap the reserved range to the memory allocation in chunks.
            u64 currentOffset = 0;
            while (currentOffset < totalSize)
            {
                u64 currentChunkSize = min(totalSize - currentOffset, committedSize);
                bool lastIteration = currentOffset + currentChunkSize >= totalSize;
                
                // First, split off a leading chunk of the original reservation, and preserve it as a placeholder.
                // This prevents other user mode applications from theoretically consuming the addresses between this operation and the next, which is the advantage that the newer function versions provide.
                if (!lastIteration)
                {
                    bool freed = VirtualFree(start + currentOffset,
                                             currentChunkSize,
                                             MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
                    
                    if (!freed)
                    {
                        ExceptWindowsLast();
                    }
                }
                
                // Then, map the freed address space range to the anonymous memory created earlier.
                PVOID remapping = mapViewOfFile3(fileMapping, 0,
                                                 start + currentOffset, 
                                                 0,							// The offset into the file mapping is always 0
                                                 currentChunkSize,
                                                 MEM_REPLACE_PLACEHOLDER,	// Replace the placeholder with the new mapping
                                                 PAGE_READWRITE,				// These pages can now be accessed as normal
                                                 0, 0);
                
                if (remapping == NULL)
                {
                    ExceptWindowsLast();
                }
                
                currentOffset += currentChunkSize;
            }
        }
        else // ... if not, we will have to use the older method
        {
            // This method is volatile, having a small chance of failure.
            const int maxAttempts = 32;
            
            bool success = false;
            
            for (int attempt = 0; attempt < maxAttempts && !success; attempt++)
            {
                // Reserve the whole range. This API does not support placeholders.
                start = (char*)VirtualAlloc(0, totalSize, MEM_RESERVE, PAGE_NOACCESS);
                
                if (start == NULL)
                {
                    ExceptWindowsLast();
                }
                
                // Free the reservation. This will ensure that a continuous block of virtual address space is available, but it does not prevent the OS from mapping other allocations there.
                VirtualFree(start, 0, MEM_RELEASE);
                
                bool couldRemap = true;
                
                u64 currentOffset = 0;
                while (currentOffset < totalSize)
                {
                    u64 currentChunkSize = min(totalSize - currentOffset, committedSize);
                    
                    // Map the address space range to the anonymous memory created earlier, and hope that it is still available.
                    LPVOID remapping = MapViewOfFileEx(fileMapping,
                                                       FILE_MAP_ALL_ACCESS,
                                                       0, 0,
                                                       currentChunkSize,
                                                       start + currentOffset);
                    
                    if (remapping == NULL)
                    {
                        couldRemap = false;
                        break;
                    }
                    
                    currentOffset += currentChunkSize;
                }
                
                // Unmap the partial mapping and try again if not successful
                if (!couldRemap)
                {
                    currentOffset = 0;
                    while (currentOffset < totalSize)
                    {
                        u64 currentChunkSize = min(totalSize - currentOffset, loop->committed);
                        
                        // Unmap each mapped section if possible
                        UnmapViewOfFile(start + currentChunkSize);
                        
                        currentOffset += currentChunkSize;
                    }
                }
                else
                {
                    success = true;
                }
            }
            
            if (!success)
            {
                Except("Could not allocate Memory Loop using legacy method, maximum number of failed attempts reached. "
                       "Consider upgrading to a newer Windows runtime. (%i attempts)",
                       maxAttempts);
            }
        }
        
        // Set the members
        loop->memory = (AnonymousMemory)fileMapping;
        loop->start = (void*)start;
        loop->committed = committedSize;
        loop->mirrored = mirroredSize;
    }
    
    // Deallocates region, returning it to an uninitialized state.
    void LoopDealloc(MemoryLoop* loop)
    {
        if (loop->start == nullptr)
            return;
        
        char* start = (char*)loop->start;
        u64 totalSize = loop->committed + loop->mirrored;
        u64 currentOffset = 0;
        while (currentOffset < totalSize)
        {
            u64 currentChunkSize = min(totalSize - currentOffset, loop->committed);
            
            // Unmap each mapped section of virtual address space
            UnmapViewOfFile(start + currentChunkSize);
            
            currentOffset += currentChunkSize;
        }
        
        // Close the file mapping, releasing the anonymous memory
        CloseHandle((HANDLE)loop->memory);
        
        loop->memory = nullptr;
        loop->start = nullptr;
        loop->committed = 0;
        loop->mirrored = 0;
    }
    
#endif
    
    //~ Memory loop Unix implementation
    
#ifdef TOOL_UNIX
    
    // Initializes/reserves uninitialized region.
    void LoopAlloc(MemoryLoop* loop, u64 minCommittedSize, u64 minMirroredSize)
    {
    }
    
    // Deallocates region, returning it to an uninitialized state.
    void LoopDealloc(MemoryLoop* loop)
    {
    }
    
#endif
    
    //~ Memory loop general implementation
    
    bool LoopIsInitialized(const MemoryLoop* loop)
    {
        return loop->start != nullptr;
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
    
    void* ArenaAllocBegin(Arena* arena, u64 reservedSize)
    {
        u64 newSize = arena->size + reservedSize;
        
        if (newSize > arena->region.committed)
        {
            if (newSize > arena->region.reserved)
            {
                Except("Cannot allocate more memory than is reserved in the Arena. (%ull + %ull > %ull)",
                       arena->size, reservedSize, arena->region.reserved);
            }
            
            u64 newCommittedSize = arena->region.committed;
            while (newSize > newCommittedSize)
            {
                newCommittedSize += TOOL_MIN(newCommittedSize, TOOL_ARENA_MAX_INCREMENT_SIZE);
            }
            
            RegionCommit(&arena->region, newCommittedSize);
        }
        
        void* result = (u8*)arena->startCurrent + arena->sizeCurrent;
        
        return result;
    }
    
    void* ArenaAllocEnd(Arena* arena, u64 actualSize)
    {
        void* head = (u8*)arena->startCurrent + arena->sizeCurrent;
        
        arena->sizeCurrent += actualSize;
        arena->size += actualSize;
        
        return head;
    }
    
    void* ArenaAlloc(Arena* arena, u64 size)
    {
        ArenaAllocBegin(arena, size);
        return ArenaAllocEnd(arena, size);
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
    
    
    
    //- Circular buffer
    
    //~ Circular buffer general implementation
    
    // Initialize and allocate a new circular buffer. The actual size and overflow region might be larger than requested.
    void CircularInit(Circular* circular, u64 requestedSize, u64 requestedOverflowSize)
    {
        LoopAlloc(&circular->loop, requestedSize, requestedOverflowSize);
        circular->start = 0;
        circular->size = 0;
    }
    
    // Allocate space within the circular buffer. Nullptr indicates insufficient space.
    void* CircularAlloc(Circular* circular, u64 size)
    {
        CircularAllocBegin(circular, size);
        return CircularAllocEnd(circular, size);
    }
    
    void* CircularAlloc(Circular* circular, u64 count, u64 size)
    {
        return CircularAlloc(circular, count * size);
    }
    
    // Allocates space in two steps, similarly to the same Arena feature.
    void* CircularAllocBegin(Circular* circular, u64 reservedSize)
    {
        if (LoopIsInitialized(&circular->loop))
        {
            Except("Cannot allocate onto a non-initialized Circular Allocator.");
        }
        
        u64 requestedSize = circular->size + reservedSize;
        u64 capacity = circular->loop.committed;
        
        if (requestedSize > capacity)
        {
            return nullptr;
        }
        
        if (circular->start + reservedSize > circular->loop.committed + circular->loop.mirrored)
        {
            Except("Cannot allocate onto Circular Allocator, "
                   "requested size does not fit into the circular buffer as a continuous region. "
                   "Consider requesting higher maximum allocation size. "
                   "(Maximum size: %llu, allocation size: %llu)",
                   circular->loop.mirrored, reservedSize);
        }
        
        // Just acquire the current circular buffer end point. Due to the looped memory mapping, reads/writes will wrap.
        u64 head = (circular->start + circular->size) % capacity;
        void* allocationLocation = ((char*)circular->loop.start) + head;
        
        return allocationLocation;
    }
    
    void* CircularAllocEnd(Circular* circular, u64 actualSize)
    {
        u64 capacity = circular->loop.committed;
        
        // Just acquire the current circular buffer end point. Due to the looped memory mapping, reads/writes will wrap.
        u64 head = (circular->start + circular->size) % capacity;
        void* allocationLocation = (char*)circular->loop.start + head;
        
        circular->size += actualSize;
        
        return allocationLocation;
    }
    
    // Get a reference to the current writing location (bookmark).
    // Can be used to then get a data pointer, or deallocate everything prior to the bookmark.
    u64 CircularGetBookmark(const Circular* circular)
    {
        u64 capacity = circular->loop.committed;
        return (circular->start + circular->size) % capacity;
    }
    
    void* CircularGetDataAt(Circular* circular, u64 bookmark)
    {
        return (char*)circular->loop.start + bookmark;
    }
    
    void CircularPopToBookmark(Circular* circular, u64 bookmark)
    {
        if (LoopIsInitialized(&circular->loop))
        {
            Except("Cannot pop a non-initialized Circular Allocator to bookmark.");
        }
        
        u64 capacity = circular->loop.committed;
        
        // The offset of the bookmark from the start
        u64 offset = (bookmark + capacity * (bookmark < circular->start) - circular->start);
        
        if (offset > circular->size)
        {
            Except("Bookmark is invalid (outside the allocated region).");
        }
        
        circular->size -= offset;
        circular->start = bookmark;
    }
    
    // Deinitialize the circular buffer.
    void CircularDeInit(Circular* circular)
    {
        LoopDealloc(&circular->loop);
        circular->start = 0;
        circular->size = 0;
    }
    
    
    
    //- Allocators
    
    //~ Allocator triggers
    
    static void* AllocationTriggerClassic(u64 size, void* data)
    {
        return ClassicAlloc(size);
    }
    
    static void DeallocationTriggerClassic(void* target, void* data)
    {
        ClassicDealloc(target);
    }
    
    static void* AllocationTriggerArena(u64 size, void* data)
    {
        return ArenaAlloc((Arena*)data, size);
    }
    
    static void* AllocationTriggerCircular(u64 size, void* data)
    {
        return CircularAlloc((Circular*)data, size);
    }
    
    //~ Exposed allocator functions
    
    MemoryAllocator Allocator() // Heap
    {
        return { &AllocationTriggerClassic, &DeallocationTriggerClassic, nullptr };
    }
    
    MemoryAllocator Allocator(Arena* arena) // Arena
    {
        return { &AllocationTriggerArena, nullptr, (void*)arena };
    }
    
    MemoryAllocator Allocator(Circular* circular) // Circular buffer
    {
        return { &AllocationTriggerCircular, nullptr, (void*)circular };
    }
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size)
    {
        return allocator.allocate(size, allocator.data);
    }
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size)
    {
        return AllocatorAlloc(allocator, count * size);
    }
    
    void AllocatorDealloc(MemoryAllocator allocator, void* target)
    {
        if (allocator.deallocate != nullptr)
        {
            return allocator.deallocate(target, allocator.data);
        }
    }
}

