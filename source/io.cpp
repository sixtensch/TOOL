
#include "io.h"
#include "text.h"

#if defined(TOOL_WINDOWS)

#include <Windows.h>

#elif defined(TOOL_UNIX)

#endif


namespace Tool
{
    //- File IO
    
    //~ File IO Windows implementation
    
#ifdef TOOL_WINDOWS
    
    static const c16* WindowsConvertPath(const c8* path)
    {
        static StringBuilder builder = {};
        if (builder.type == StringTypeNone)
        {
            StringBuilderInit(&builder, 1024, StringTypeUTF16);
        }
        
        StringBuilderReset(&builder);
        StringBuilderAdd(&builder, path);
        
        c16* current = builder.str16;
        while (*current != L'\0')
        {
            b8 forwardSlash = *current == L'/';
            *current = 
                forwardSlash * '\\' +
                !forwardSlash * (*current);
            current++;
        }
        
        return builder.str16;
    }
    
    b8 FileOpen(File* outFile, const c8* filename, OpenMode mode, i32 flags)
    {
        File file = 0;
        HANDLE* handle = (HANDLE*)&file;
        
        u32 disposition = 0;
        b8 eraseFile = false;
        b8 append = false;
        
        switch (mode)
        {
            case OpenModeRead:              disposition = OPEN_EXISTING; break; 
            case OpenModeNew:               disposition = CREATE_NEW; break;
            case OpenModeNewOrAppend:       disposition = OPEN_ALWAYS; append = true; break;
            case OpenModeNewOrOverwrite:    disposition = CREATE_ALWAYS; break;
            case OpenModeAppendExisting:    disposition = OPEN_EXISTING; append = true; break;
            case OpenModeOverwriteExisting: disposition = TRUNCATE_EXISTING; break;
        }
        
        const c16* parsedFilename = WindowsConvertPath(filename);
        
        u32 access = GENERIC_READ | (GENERIC_WRITE * (mode != OpenModeRead));
        u32 shareMode = 
            FILE_SHARE_READ * (flags & OpenFlagsShareRead != 0) & 
            FILE_SHARE_WRITE * (flags & OpenFlagsShareWrite != 0);
        u32 flagsAndAttributes = 
            FILE_ATTRIBUTE_NORMAL &
            FILE_FLAG_NO_BUFFERING * (flags & OpenFlagsUnbuffered != 0) &
            FILE_FLAG_OVERLAPPED * (flags & 0/*OpenFlagsAsynchronous*/ != 0);
        
        *handle = CreateFileW((wchar_t*)parsedFilename, access, shareMode, 
                              nullptr, disposition, flagsAndAttributes, nullptr);
        
        if (*handle == INVALID_HANDLE_VALUE)
        {
            *outFile = 0;
            return false;
        }
        else
        {
            if (append)
            {
                SetFilePointer(*handle, 0, nullptr, FILE_END);
            }
            
            *outFile = file;
            return true;
        }
    }
    
    b8 FileExists(const c8* filename)
    {
        const c16* parsedFilename = WindowsConvertPath(filename);
        
        u32 attributes = GetFileAttributesW((wchar_t*)parsedFilename);
        
        return (attributes != INVALID_FILE_ATTRIBUTES && 
                !(attributes & FILE_ATTRIBUTE_DIRECTORY));
    }
    
    void FileClose(File file)
    {
        if (file != 0)
        {
            return;
        }
        
        HANDLE* handle = (HANDLE*)&file;
        CloseHandle(*handle);
    }
    
    u64 FileSize(File file)
    {
        if (file == 0)
        {
            return 0;
        }
        
        HANDLE* handle = (HANDLE*)&file;
        i64 size = 0;
        b8 success = GetFileSizeEx(*handle, (LARGE_INTEGER*)&size);
        return (u64)size;
    }
    
    void FileRead(File file, void* destination, u32 size, u32* actual)
    {
        if (file == 0)
        {
            return;
        }
        
        HANDLE* handle = (HANDLE*)&file;
        ReadFile(*handle, destination, size, (LPDWORD)actual, nullptr);
    }
    
    void FileWrite(File file, const void* source, u32 size, u32* actual)
    {
        if (file == 0)
        {
            return;
        }
        
        HANDLE* handle = (HANDLE*)&file;
        WriteFile(*handle, source, size, (LPDWORD)actual, nullptr);
    }
    
#endif 
    
}