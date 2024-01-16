#include "threading.h"



//~ Platform-agnostic stuff

#if defined(TOOL_UNIX)

#include <sys/types.h>
#define TOOL_THREAD_T pthread_t

#elif defined(TOOL_WINDOWS)

#include <Windows.h>
#define TOOL_THREAD_T HANDLE

#endif
