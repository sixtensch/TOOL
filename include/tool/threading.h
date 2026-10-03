#ifndef THREADING_H
#define THREADING_H

#include "basics.h"
#include "atomic.h"



//~ Threading
//
// Threads, and primitives for coordinating them. Every primitive is a few atomics in plain memory: zero-initialized
// is ready to use, nothing needs destroying, and they can live anywhere, but must not move while in use. Only a
// thread that has to wait involves the operating system. Each type has the same size and alignment on every platform.
//
// Timeouts are in milliseconds. A timeout of 0 tries once without waiting.
//
// Waits on conditions and atomics may return without being woken. Wait in a loop that checks what you're waiting for.



namespace Tool
{
	//- Types

	// An operating system thread handle, 0 for none.
	typedef u64 Thread;

	typedef void (*ThreadFunction)(void* data);

	// Not recursive: a thread that locks a mutex it already holds deadlocks. Debug builds assert on that, and on
	// unlocking a mutex the calling thread doesn't hold.
	struct Mutex
	{
		AtomicU32 state;
		AtomicU32 owner; // The holding thread's ID in debug builds
	};

	struct Condition
	{
		AtomicU32 sequence;
	};

	// Many readers or one writer. Waiting writers hold back new readers, so writers can't starve.
	struct RWLock
	{
		AtomicU32 state;
		AtomicU32 writerWake;
	};

	// A count of 0 when zero-initialized. Post the starting count, if any.
	struct Semaphore
	{
		AtomicU32 count;
		AtomicU32 waiters;
	};

	// Holds threads until 'count' of them have arrived, then releases them all and starts the next round. Needs
	// BarrierInit, for its count.
	struct Barrier
	{
		u32 count;
		AtomicU32 arrived;
		AtomicU32 generation;
	};

	static_assert(sizeof(Mutex) == 8 && alignof(Mutex) == 4, "Mutex layout");
	static_assert(sizeof(Condition) == 4 && alignof(Condition) == 4, "Condition layout");
	static_assert(sizeof(RWLock) == 8 && alignof(RWLock) == 4, "RWLock layout");
	static_assert(sizeof(Semaphore) == 8 && alignof(Semaphore) == 4, "Semaphore layout");
	static_assert(sizeof(Barrier) == 12 && alignof(Barrier) == 4, "Barrier layout");



	//- Functions

	//~ Thread

	// Returns 0 on failure. The thread runs until 'function' returns, and must then be joined or detached.
	Thread ThreadCreate(ThreadFunction function, void* data);

	void ThreadDetach(Thread thread);
	void ThreadJoin(Thread thread);
	b8 ThreadTryJoin(Thread thread, u32 timeout = 0); // True if the thread ended and was joined

	void ThreadSleep(u32 milliseconds);
	void ThreadYield();                 // Offers the rest of this thread's time slice to other threads
	void ThreadSetName(const c8* name); // Names the calling thread for debuggers and profilers. Linux keeps 15 bytes.
	u32 ThreadCurrentId();              // The operating system's ID of the calling thread, never 0

	u32 ProcessorCount(); // Logical processors

	//~ Waiting on atomics

	void AtomicU32Wait(AtomicU32* atomic, u32 expected);                // Sleeps while the atomic holds expected
	b8 AtomicU32TryWait(AtomicU32* atomic, u32 expected, u32 timeout); // False on timeout
	void AtomicU32WakeOne(AtomicU32* atomic);
	void AtomicU32WakeAll(AtomicU32* atomic);

	//~ Mutex

	void MutexLock(Mutex* mutex);
	b8 MutexTryLock(Mutex* mutex, u32 timeout = 0); // True if locked
	void MutexUnlock(Mutex* mutex);

	//~ Condition
	// Waits unlock the mutex while asleep, and hold it again when they return, timed out or not.

	void ConditionWait(Condition* condition, Mutex* mutex);
	b8 ConditionTryWait(Condition* condition, Mutex* mutex, u32 timeout); // False on timeout
	void ConditionWakeOne(Condition* condition);
	void ConditionWakeAll(Condition* condition);

	//~ RWLock

	void RWLockLockRead(RWLock* lock);
	b8 RWLockTryLockRead(RWLock* lock, u32 timeout = 0); // True if locked
	void RWLockUnlockRead(RWLock* lock);

	void RWLockLockWrite(RWLock* lock);
	b8 RWLockTryLockWrite(RWLock* lock, u32 timeout = 0); // True if locked
	void RWLockUnlockWrite(RWLock* lock);

	//~ Semaphore

	void SemaphorePost(Semaphore* semaphore, u32 count = 1);
	void SemaphoreWait(Semaphore* semaphore);
	b8 SemaphoreTryWait(Semaphore* semaphore, u32 timeout = 0); // True if decremented

	//~ Barrier

	void BarrierInit(Barrier* barrier, u32 count);
	b8 BarrierWait(Barrier* barrier); // True on exactly one thread per round, for work done once between rounds
} //namespace Tool

#endif //THREADING_H
