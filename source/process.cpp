#include "process.h"
#include "error.h"



//~ Windows
#if TOOL_WINDOWS

#include <Windows.h>

//~ Unix
#elif TOOL_UNIX

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#endif



namespace Tool
{
#if TOOL_WINDOWS

    //- Windows

    // Longest command line Windows takes, in UTF-16 units, terminator included
    static constexpr u32 CommandLineCapacity = 32768;
    static constexpr u32 DirectoryCapacity = 1024;

    // Appends one UTF-8 argument, quoted as programs parse their command lines: backslashes count only before a
    // quote, where each is doubled. False if it doesn't fit or isn't valid.
    static b8 CommandLineAppend(wchar_t* line, u32* length, const c8* argument, b8 path)
    {
        i32 size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argument, -1, nullptr, 0);
        if (size <= 0 || (u32)size >= CommandLineCapacity)
            return false;

        wchar_t* wide = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (u64)size * sizeof(wchar_t));
        if (wide == nullptr)
            return false;
        MultiByteToWideChar(CP_UTF8, 0, argument, -1, wide, size);

        u32 at = *length;
        b8 fits = true;
        auto put = [&](wchar_t c)
        {
            if (at + 1 >= CommandLineCapacity)
                fits = false;
            else
                line[at++] = c;
        };

        // Plain arguments stay unquoted, as some programs, such as cmd, read the line themselves
        b8 quote = wide[0] == 0;
        for (const wchar_t* c = wide; *c != 0; c++)
        {
            if (*c == L' ' || *c == L'\t' || *c == L'"')
                quote = true;
        }

        if (at > 0)
            put(L' ');
        if (quote)
            put(L'"');

        u32 backslashes = 0;
        for (const wchar_t* c = wide; *c != 0; c++)
        {
            wchar_t character = (path && *c == L'/') ? L'\\' : *c;

            if (character == L'\\')
            {
                backslashes++;
            }
            else
            {
                u32 repeat = character == L'"' ? backslashes * 2 + 1 : backslashes;
                for (u32 i = 0; i < repeat; i++)
                    put(L'\\');
                backslashes = 0;
                put(character);
            }
        }

        for (u32 i = 0; i < (quote ? backslashes * 2 : backslashes); i++)
            put(L'\\');
        if (quote)
            put(L'"');

        HeapFree(GetProcessHeap(), 0, wide);

        if (fits)
            *length = at;
        return fits;
    }

    b8 ProcessStart(Process* process, const c8* const* arguments, u32 argumentCount, const c8* directory)
    {
        *process = {};

        if (argumentCount == 0)
            return TOOL_FAIL("No program to start");

        wchar_t* line = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, CommandLineCapacity * sizeof(wchar_t));
        if (line == nullptr)
            return TOOL_FAIL("Out of memory for a command line");

        u32 length = 0;
        line[0] = 0;
        for (u32 i = 0; i < argumentCount; i++)
        {
            if (!CommandLineAppend(line, &length, arguments[i], i == 0))
            {
                HeapFree(GetProcessHeap(), 0, line);
                return TOOL_FAIL("Could not make a command line for %.64s", arguments[0]);
            }
        }
        line[length] = 0;

        wchar_t wideDirectory[DirectoryCapacity];
        if (directory != nullptr)
        {
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, directory, -1, wideDirectory, DirectoryCapacity) <= 0)
            {
                HeapFree(GetProcessHeap(), 0, line);
                return TOOL_FAIL("Unusable directory: %.64s", directory);
            }

            for (wchar_t* c = wideDirectory; *c != 0; c++)
            {
                if (*c == L'/')
                    *c = L'\\';
            }
        }

        // Only the program gets the write end, so the read ends once it, and whatever it started, has gone
        SECURITY_ATTRIBUTES inherit = {};
        inherit.nLength = sizeof(inherit);
        inherit.bInheritHandle = TRUE;

        HANDLE read = nullptr;
        HANDLE write = nullptr;
        if (!CreatePipe(&read, &write, &inherit, 0))
        {
            HeapFree(GetProcessHeap(), 0, line);
            return TOOL_FAIL("Could not create a pipe (error %lu)", GetLastError());
        }
        SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0);

        HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, nullptr);

        // Handles are named one by one, so other processes being started at the same time don't catch this pipe
        HANDLE inherited[2] = { write, input };
        u64 attributeSize = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
        LPPROC_THREAD_ATTRIBUTE_LIST attributes = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeSize);

        b8 started = false;
        PROCESS_INFORMATION information = {};
        if (attributes != nullptr && InitializeProcThreadAttributeList(attributes, 1, 0, &attributeSize))
        {
            UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited), nullptr, nullptr);

            STARTUPINFOEXW startup = {};
            startup.StartupInfo.cb = sizeof(startup);
            startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdInput = input;
            startup.StartupInfo.hStdOutput = write;
            startup.StartupInfo.hStdError = write;
            startup.lpAttributeList = attributes;

            started = CreateProcessW(nullptr, line, nullptr, nullptr, TRUE, CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                                     directory != nullptr ? wideDirectory : nullptr, &startup.StartupInfo, &information) != 0;
            DeleteProcThreadAttributeList(attributes);
        }

        DWORD error = GetLastError();

        if (attributes != nullptr)
            HeapFree(GetProcessHeap(), 0, attributes);
        HeapFree(GetProcessHeap(), 0, line);
        CloseHandle(write);
        CloseHandle(input);

        if (!started)
        {
            CloseHandle(read);
            return TOOL_FAIL("Could not start %.64s (error %lu)", arguments[0], error);
        }

        CloseHandle(information.hThread);

        process->handle = (u64)information.hProcess;
        process->output = (u64)read;
        return true;
    }

    b8 ProcessRead(Process* process, void* destination, u64 capacity, u64* outRead)
    {
        *outRead = 0;

        DWORD size = capacity > 0x7FFFFFFF ? 0x7FFFFFFF : (DWORD)capacity;
        DWORD read = 0;
        if (!ReadFile((HANDLE)process->output, destination, size, &read, nullptr))
        {
            // The program's end, seen from a pipe
            if (GetLastError() == ERROR_BROKEN_PIPE)
                return true;

            return TOOL_FAIL("Could not read from a process (error %lu)", GetLastError());
        }

        *outRead = read;
        return true;
    }

    b8 ProcessWait(Process* process, i32* outExitCode)
    {
        if (WaitForSingleObject((HANDLE)process->handle, INFINITE) != WAIT_OBJECT_0)
            return TOOL_FAIL("Could not wait for a process (error %lu)", GetLastError());

        DWORD code = 0;
        if (!GetExitCodeProcess((HANDLE)process->handle, &code))
            return TOOL_FAIL("Could not read a process's exit code (error %lu)", GetLastError());

        *outExitCode = (i32)code;
        return true;
    }

    void ProcessKill(Process* process)
    {
        if (process->handle != 0)
            TerminateProcess((HANDLE)process->handle, 1);
    }

    void ProcessClose(Process* process)
    {
        if (process->output != 0)
            CloseHandle((HANDLE)process->output);
        if (process->handle != 0)
            CloseHandle((HANDLE)process->handle);

        *process = {};
    }

#elif TOOL_UNIX && !TOOL_WEB

    //- Unix

    // Stored in Process::output, as 0 means none
    static constexpr u64 DescriptorOffset = 1;

    b8 ProcessStart(Process* process, const c8* const* arguments, u32 argumentCount, const c8* directory)
    {
        *process = {};

        if (argumentCount == 0)
            return TOOL_FAIL("No program to start");

        c8** list = (c8**)malloc(((u64)argumentCount + 1) * sizeof(c8*));
        if (list == nullptr)
            return TOOL_FAIL("Out of memory for an argument list");

        for (u32 i = 0; i < argumentCount; i++)
            list[i] = (c8*)arguments[i];
        list[argumentCount] = nullptr;

        // The second pipe tells why the program couldn't start: it closes unused once exec succeeds
        i32 output[2];
        i32 status[2];
        if (pipe(output) != 0)
        {
            free(list);
            return TOOL_FAIL("Could not create a pipe: %s", strerror(errno));
        }
        if (pipe(status) != 0)
        {
            close(output[0]);
            close(output[1]);
            free(list);
            return TOOL_FAIL("Could not create a pipe: %s", strerror(errno));
        }
        fcntl(output[0], F_SETFD, FD_CLOEXEC);
        fcntl(status[0], F_SETFD, FD_CLOEXEC);
        fcntl(status[1], F_SETFD, FD_CLOEXEC);

        pid_t child = fork();
        if (child < 0)
        {
            i32 error = errno;
            close(output[0]);
            close(output[1]);
            close(status[0]);
            close(status[1]);
            free(list);
            return TOOL_FAIL("Could not start %.64s: %s", arguments[0], strerror(error));
        }

        if (child == 0)
        {
            i32 input = open("/dev/null", O_RDONLY);
            dup2(input, 0);
            dup2(output[1], 1);
            dup2(output[1], 2);
            close(output[1]);

            if (directory == nullptr || chdir(directory) == 0)
                execvp(list[0], list);

            i32 error = errno;
            ssize_t written = write(status[1], &error, sizeof(error));
            (void)written;
            _exit(127);
        }

        close(output[1]);
        close(status[1]);
        free(list);

        i32 error = 0;
        ssize_t got = read(status[0], &error, sizeof(error));
        close(status[0]);

        if (got == (ssize_t)sizeof(error))
        {
            close(output[0]);
            waitpid(child, nullptr, 0);
            return TOOL_FAIL("Could not start %.64s: %s", arguments[0], strerror(error));
        }

        process->handle = (u64)child;
        process->output = (u64)output[0] + DescriptorOffset;
        return true;
    }

    b8 ProcessRead(Process* process, void* destination, u64 capacity, u64* outRead)
    {
        *outRead = 0;

        for (;;)
        {
            ssize_t got = read((i32)(process->output - DescriptorOffset), destination, capacity);
            if (got >= 0)
            {
                *outRead = (u64)got;
                return true;
            }

            if (errno != EINTR)
                return TOOL_FAIL("Could not read from a process: %s", strerror(errno));
        }
    }

    b8 ProcessWait(Process* process, i32* outExitCode)
    {
        i32 status = 0;
        for (;;)
        {
            if (waitpid((pid_t)process->handle, &status, 0) >= 0)
                break;

            if (errno != EINTR)
                return TOOL_FAIL("Could not wait for a process: %s", strerror(errno));
        }

        *outExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        process->handle = 0;
        return true;
    }

    void ProcessKill(Process* process)
    {
        if (process->handle != 0)
            kill((pid_t)process->handle, SIGKILL);
    }

    void ProcessClose(Process* process)
    {
        if (process->output != 0)
            close((i32)(process->output - DescriptorOffset));

        *process = {};
    }

#else

    //- No processes

    b8 ProcessStart(Process* process, const c8* const* arguments, u32 argumentCount, const c8* directory)
    {
        *process = {};
        return TOOL_FAIL("Programs can't be started here");
    }

    b8 ProcessRead(Process* process, void* destination, u64 capacity, u64* outRead)
    {
        *outRead = 0;
        return true;
    }

    b8 ProcessWait(Process* process, i32* outExitCode)
    {
        return TOOL_FAIL("Programs can't be started here");
    }

    void ProcessKill(Process* process)
    {
    }

    void ProcessClose(Process* process)
    {
        *process = {};
    }

#endif
}
