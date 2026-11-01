#ifndef _TOOL_PROCESS_H
#define _TOOL_PROCESS_H

#include "basics.h"



//~ Processes
//
// Starts another program, reads what it prints, and waits for it to end. Its standard output and error come merged
// through one stream, and its standard input is empty. Text is UTF-8. Arguments go to the program exactly as given,
// so nothing needs quoting for a shell, and no shell is involved. Functions that can fail return false and record why
// in the thread's error record (error.h). Programs can't be started on the web, where ProcessStart always fails.
//
// Process process = {};
// const c8* arguments[] = { "tool.exe", "--name", "a value" };
// if (ProcessStart(&process, arguments, 3))
// {
//     u64 read;
//     while (ProcessRead(&process, buffer, sizeof(buffer), &read) && read > 0)
//         ...
//
//     i32 code;
//     ProcessWait(&process, &code);
//     ProcessClose(&process);
// }



namespace Tool
{
    //- Types

    // A started program. Zeroed is none.
    struct Process
    {
        u64 handle;
        u64 output;
    };



    //- Functions

    // Start a program: arguments[0] is its path, or a name found through the system's search path. It runs in
    // 'directory', or in this program's working directory for null. False if it could not be started.
    b8 ProcessStart(Process* process, const c8* const* arguments, u32 argumentCount, const c8* directory = nullptr);

    // Read what the program has printed, waiting for some if there is none yet. 'outRead' is 0 once it has ended and
    // everything was read. A program printing more than is read waits for it, so read until then before waiting.
    b8 ProcessRead(Process* process, void* destination, u64 capacity, u64* outRead);

    // Wait for the program to end, and report its exit code.
    b8 ProcessWait(Process* process, i32* outExitCode);

    // End the program now, without its cooperation. It still has to be waited for or closed.
    void ProcessKill(Process* process);

    // Release the process. The program goes on running if it hasn't ended.
    void ProcessClose(Process* process);
}

#endif //_TOOL_PROCESS_H
