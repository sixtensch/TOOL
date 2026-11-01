#include "tool.h"
#include "test.h"
#include <string.h>

using namespace Tool;



#if !TOOL_WEB

#if TOOL_WINDOWS
static const c8* shell = "cmd";
static const c8* shellRun = "/c";
#else
static const c8* shell = "sh";
static const c8* shellRun = "-c";
#endif

// Runs a shell line to its end, collecting everything it prints. Returns its exit code.
static i32 Run(const c8* line, c8* output, u64 capacity, const c8* directory = nullptr)
{
	const c8* arguments[] = { shell, shellRun, line };

	Process process = {};
	TOOL_ASSERT(ProcessStart(&process, arguments, 3, directory));

	u64 used = 0;
	for (;;)
	{
		u64 read = 0;
		TOOL_ASSERT(ProcessRead(&process, output + used, capacity - 1 - used, &read));
		if (read == 0)
			break;
		used += read;
	}
	output[used] = 0;

	i32 code = -1;
	TOOL_ASSERT(ProcessWait(&process, &code));
	ProcessClose(&process);
	return code;
}

#endif

void TestProcess()
{
#if !TOOL_WEB
	c8 output[4096];

	// What it prints, on either stream, and its exit code
	TOOL_ASSERT(Run("echo one two", output, sizeof(output)) == 0);
	TOOL_ASSERT(strstr(output, "one two") != nullptr);

	TOOL_ASSERT(Run("echo problem 1>&2", output, sizeof(output)) == 0);
	TOOL_ASSERT(strstr(output, "problem") != nullptr);

	TOOL_ASSERT(Run("exit 3", output, sizeof(output)) == 3);

	// More than a pipe holds at once
#if TOOL_WINDOWS
	const c8* many = "for /L %i in (1,1,2000) do @echo line %i";
#else
	const c8* many = "i=0; while [ $i -lt 2000 ]; do echo line $i; i=$((i+1)); done";
#endif
	static c8 large[1024 * 1024];
	TOOL_ASSERT(Run(many, large, sizeof(large)) == 0);
	TOOL_ASSERT(strstr(large, "line 2000") != nullptr || strstr(large, "line 1999") != nullptr);

	// The folder it runs in
	const c8* folder = "tooltest_process_folder";
	TOOL_ASSERT(DirectoryCreate(folder));
#if TOOL_WINDOWS
	TOOL_ASSERT(Run("cd", output, sizeof(output), folder) == 0);
#else
	TOOL_ASSERT(Run("pwd", output, sizeof(output), folder) == 0);
#endif
	TOOL_ASSERT(strstr(output, folder) != nullptr);
	TOOL_ASSERT(DirectoryDelete(folder));

	// A program that isn't there fails to start, and leaves nothing behind
	const c8* missing[] = { "tooltest_no_such_program" };
	Process process = {};
	TOOL_ASSERT(!ProcessStart(&process, missing, 1));
	TOOL_ASSERT(process.handle == 0 && process.output == 0);

	// Ended from outside
#if TOOL_WINDOWS
	const c8* endless[] = { "cmd", "/c", "ping -n 60 127.0.0.1 > NUL" };
#else
	const c8* endless[] = { "sleep", "60" };
#endif
	TOOL_ASSERT(ProcessStart(&process, endless, sizeof(endless) / sizeof(endless[0])));
	ProcessKill(&process);
	i32 code = 0;
	TOOL_ASSERT(ProcessWait(&process, &code));
	TOOL_ASSERT(code != 0);
	ProcessClose(&process);
#endif

	printf("Process: passed\n");
}
