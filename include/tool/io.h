#ifndef _TOOL_IO_H
#define _TOOL_IO_H

#include "basics.h"
#include "memory.h"



namespace Tool
{
    //- Types 
    
    //~ File
    
    typedef u64 File;
    
    enum OpenMode
    {
        // Read modes
        OpenModeRead,                     // Fails if file does not exist
        
        // Read/write modes
        OpenModeNew,                     // Fails if file exists
        OpenModeNewOrAppend,             // Creates file, or appends if one exists
        OpenModeNewOrOverwrite,          // Creates file, or overwrites if one exists
        OpenModeAppendExisting,          // Fails if file does not exist. Appends on write
        OpenModeOverwriteExisting,       // Fails if file does not exist. Overwrites on write
    };
    
    enum OpenFlags
    {
        // Unbuffered reads have more strict requirements
        // Reads must be aligned to sector boundaries
        OpenFlagsUnbuffered   = 1 << 0,
        
        // Allow overlapping asynchronous access to the opened file
        // By default, when this flag is not set, access operations are serialized
        //OpenFlagsAsynchronous = 1 << 1,
        
        // Allows subsequent calls that also set this flag to read from the same, already
        // opened file
        OpenFlagsShareRead    = 1 << 2,
        
        // Allows subsequent calls that also set this flag to write to the same, already
        // opened file
        OpenFlagsShareWrite   = 1 << 3
    };
    
    
    
    //- Usage and helper functions
    
    //~ File IO
    
    // Returns true if the file can be opened/created successfully with the given mode
    b8 FileOpen(File* outFile, const c8* filename, OpenMode mode, i32 flags = 0);
    b8 FileExists(const c8* filename);
    
    void FileClose(File file);
    
    u64 FileSize(File file);
    void FileRead(File file, void* destination, u32 size, u32* outSize = nullptr);
    void FileWrite(File file, const void* source, u32 size, u32* outSize = nullptr);
    
    b8 FileDump(const c8* filename, void** outDump, u32* outSize, MemoryAllocator allocator);
    
}

#endif //_TOOL_IO_H
