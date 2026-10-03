#include "threading.h"
#include "temporal.h"
#include "error.h"



//~ Windows
#if defined(TOOL_WINDOWS)

#include <Windows.h>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

//~ Unix
#elif defined(TOOL_UNIX)

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#if defined(TOOL_WEB)
#include <emscripten/threading.h>
#elif defined(__linux__)
#include <linux/futex.h>
#include <sys/syscall.h>
#else
#error "No address wait for this platform"
#endif

#endif



namespace Tool
{
    //- Platform-agnostic helpers

    // A timeout of this many milliseconds never ends. Windows' INFINITE has the same value.
    static constexpr u32 WaitForever = U32_MAX;

    // What's left of 'timeout' since 'start', for waits that may wake before anything happened.
    static u32 WaitRemaining(Timepoint start, u32 timeout)
    {
        if (timeout == WaitForever || timeout == 0)
            return timeout;

        i64 elapsed = NanosecondsSince(start) / 1000000;
        return elapsed >= (i64)timeout ? 0 : (u32)(timeout - elapsed);
    }

    static Timepoint WaitStart(u32 timeout)
    {
        return timeout == WaitForever || timeout == 0 ? 0 : TimepointNow();
    }

    // A hint to the processor inside a spin loop.
    static inline void SpinPause()
    {
#if defined(_MSC_VER) && !defined(__clang__)
#if defined(_M_ARM64) || defined(_M_ARM64EC)
        __yield();
#else
        _mm_pause();
#endif
#elif defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
        __asm__ __volatile__("yield");
#endif
    }

    static constexpr u32 SpinCount = 100;



    //- Address waiting
    // Sleeps while the atomic holds 'expected', at most 'timeout' milliseconds. False only on timeout.
    // The wakes report whether they surely woke a thread; false may still have woken one.

#if defined(TOOL_WINDOWS)

    static b8 AddressWait(AtomicU32* atomic, u32 expected, u32 timeout)
    {
        if (WaitOnAddress(&atomic->value, &expected, sizeof(u32), timeout))
            return true;

        return GetLastError() != ERROR_TIMEOUT;
    }

    static b8 AddressWakeOne(AtomicU32* atomic)
    {
        WakeByAddressSingle(&atomic->value);
        return false;
    }

    static void AddressWakeAll(AtomicU32* atomic)
    {
        WakeByAddressAll(&atomic->value);
    }

#elif defined(TOOL_WEB)

    static b8 AddressWait(AtomicU32* atomic, u32 expected, u32 timeout)
    {
        f64 milliseconds = timeout == WaitForever ? __builtin_inf() : (f64)timeout;
        return emscripten_futex_wait(&atomic->value, expected, milliseconds) != -ETIMEDOUT;
    }

    static b8 AddressWakeOne(AtomicU32* atomic)
    {
        return emscripten_futex_wake(&atomic->value, 1) > 0;
    }

    static void AddressWakeAll(AtomicU32* atomic)
    {
        emscripten_futex_wake(&atomic->value, I32_MAX);
    }

#elif defined(TOOL_UNIX)

    static b8 AddressWait(AtomicU32* atomic, u32 expected, u32 timeout)
    {
        timespec relative = {};
        timespec* limit = nullptr;
        if (timeout != WaitForever)
        {
            relative.tv_sec = timeout / 1000;
            relative.tv_nsec = (long)(timeout % 1000) * 1000000;
            limit = &relative;
        }

        long result = syscall(SYS_futex, &atomic->value, FUTEX_WAIT_PRIVATE, expected, limit, nullptr, 0);
        return result == 0 || errno != ETIMEDOUT;
    }

    static b8 AddressWakeOne(AtomicU32* atomic)
    {
        return syscall(SYS_futex, &atomic->value, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) > 0;
    }

    static void AddressWakeAll(AtomicU32* atomic)
    {
        syscall(SYS_futex, &atomic->value, FUTEX_WAKE_PRIVATE, I32_MAX, nullptr, nullptr, 0);
    }

#endif

    void AtomicU32Wait(AtomicU32* atomic, u32 expected)
    {
        AddressWait(atomic, expected, WaitForever);
    }

    b8 AtomicU32TryWait(AtomicU32* atomic, u32 expected, u32 timeout)
    {
        return AddressWait(atomic, expected, timeout);
    }

    void AtomicU32WakeOne(AtomicU32* atomic)
    {
        AddressWakeOne(atomic);
    }

    void AtomicU32WakeAll(AtomicU32* atomic)
    {
        AddressWakeAll(atomic);
    }



    //- Thread

    // Lives on the creating thread's stack, which waits until the new thread has copied it.
    struct ThreadStart
    {
        ThreadFunction function;
        void* data;
        AtomicU32 started;
    };

    static void ThreadRun(ThreadStart* start)
    {
        ThreadFunction function = start->function;
        void* data = start->data;

        // The creator may return and free 'start' as soon as this lands. Waking a freed address wakes nothing.
        AtomicU32Store(&start->started, 1);
        AddressWakeOne(&start->started);

        function(data);
    }

    static void ThreadAwaitStart(ThreadStart* start)
    {
        while (AtomicU32Load(&start->started) == 0)
            AddressWait(&start->started, 0, WaitForever);
    }

    static thread_local u32 currentThreadId = 0;

    //~ Thread Windows implementation

#if defined(TOOL_WINDOWS)

    static DWORD WINAPI ThreadEntry(LPVOID parameter)
    {
        ThreadRun((ThreadStart*)parameter);
        return 0;
    }

    Thread ThreadCreate(ThreadFunction function, void* data)
    {
        ThreadStart start = { function, data, {} };

        HANDLE handle = CreateThread(nullptr, 0, ThreadEntry, &start, 0, nullptr);
        if (handle == nullptr)
        {
            TOOL_FAIL_WINDOWS();
            return 0;
        }

        ThreadAwaitStart(&start);
        return (Thread)(UINT_PTR)handle;
    }

    void ThreadDetach(Thread thread)
    {
        CloseHandle((HANDLE)(UINT_PTR)thread);
    }

    void ThreadJoin(Thread thread)
    {
        HANDLE handle = (HANDLE)(UINT_PTR)thread;

        DWORD result = WaitForSingleObject(handle, INFINITE);
        TOOL_ASSERT(result == WAIT_OBJECT_0, "Could not join Thread (Windows error %lu)", GetLastError());

        CloseHandle(handle);
    }

    b8 ThreadTryJoin(Thread thread, u32 timeout)
    {
        HANDLE handle = (HANDLE)(UINT_PTR)thread;

        DWORD result = WaitForSingleObject(handle, timeout);
        if (result == WAIT_TIMEOUT)
            return false;

        TOOL_ASSERT(result == WAIT_OBJECT_0, "Could not join Thread (Windows error %lu)", GetLastError());
        CloseHandle(handle);
        return true;
    }

    void ThreadSleep(u32 milliseconds)
    {
        Sleep(milliseconds);
    }

    void ThreadYield()
    {
        SwitchToThread();
    }

    void ThreadSetName(const c8* name)
    {
        wchar_t wide[256];
        if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 256) == 0)
        {
            TOOL_FAIL_WINDOWS();
            return;
        }

        SetThreadDescription(GetCurrentThread(), wide);
    }

    u32 ThreadCurrentId()
    {
        if (currentThreadId == 0)
            currentThreadId = (u32)GetCurrentThreadId();

        return currentThreadId;
    }

    u32 ProcessorCount()
    {
        return (u32)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    }

#endif

    //~ Thread Unix implementation

#if defined(TOOL_UNIX)

    static void* ThreadEntry(void* parameter)
    {
        ThreadRun((ThreadStart*)parameter);
        return nullptr;
    }

    static pthread_t ThreadHandle(Thread thread)
    {
        return (pthread_t)(uintptr_t)thread;
    }

    Thread ThreadCreate(ThreadFunction function, void* data)
    {
        ThreadStart start = { function, data, {} };

        pthread_t thread = {};
        i32 result = pthread_create(&thread, nullptr, ThreadEntry, &start);
        if (result != 0)
        {
            errno = result;
            TOOL_FAIL_ERRNO();
            return 0;
        }

        ThreadAwaitStart(&start);
        return (Thread)(uintptr_t)thread;
    }

    void ThreadDetach(Thread thread)
    {
        pthread_detach(ThreadHandle(thread));
    }

    void ThreadJoin(Thread thread)
    {
        i32 result = pthread_join(ThreadHandle(thread), nullptr);
        TOOL_ASSERT(result == 0, "Could not join Thread (error %d)", result);
    }

    b8 ThreadTryJoin(Thread thread, u32 timeout)
    {
        if (timeout == WaitForever)
        {
            ThreadJoin(thread);
            return true;
        }

        i32 result = 0;
        if (timeout == 0)
        {
            result = pthread_tryjoin_np(ThreadHandle(thread), nullptr);
        }
        else
        {
            // The deadline is on the wall clock, so the wait stretches or shrinks if the clock is set meanwhile.
            timespec deadline = {};
            clock_gettime(CLOCK_REALTIME, &deadline);
            deadline.tv_sec += timeout / 1000;
            deadline.tv_nsec += (long)(timeout % 1000) * 1000000;
            if (deadline.tv_nsec >= 1000000000)
            {
                deadline.tv_sec++;
                deadline.tv_nsec -= 1000000000;
            }

            result = pthread_timedjoin_np(ThreadHandle(thread), nullptr, &deadline);
        }

        if (result == EBUSY || result == ETIMEDOUT)
            return false;

        TOOL_ASSERT(result == 0, "Could not join Thread (error %d)", result);
        return true;
    }

    void ThreadSleep(u32 milliseconds)
    {
        timespec remaining = {};
        remaining.tv_sec = milliseconds / 1000;
        remaining.tv_nsec = (long)(milliseconds % 1000) * 1000000;

        // Signals cut a sleep short, leaving the rest in 'remaining'
        while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR) {}
    }

    void ThreadYield()
    {
        sched_yield();
    }

    void ThreadSetName(const c8* name)
    {
        // Linux takes 15 bytes and a terminator. Cut at a character boundary.
        c8 truncated[16] = {};
        u32 length = 0;
        while (length < 15 && name[length] != 0)
            length++;

        if (name[length] != 0)
            while (length > 0 && ((u8)name[length] & 0xC0) == 0x80)
                length--;

        for (u32 i = 0; i < length; i++)
            truncated[i] = name[i];

#if defined(TOOL_WEB)
        emscripten_set_thread_name(pthread_self(), truncated);
#else
        pthread_setname_np(pthread_self(), truncated);
#endif
    }

    u32 ThreadCurrentId()
    {
        if (currentThreadId == 0)
        {
#if defined(TOOL_WEB)
            currentThreadId = (u32)(uintptr_t)pthread_self();
#else
            currentThreadId = (u32)syscall(SYS_gettid);
#endif
        }

        return currentThreadId;
    }

    u32 ProcessorCount()
    {
        long count = sysconf(_SC_NPROCESSORS_ONLN);
        return count > 0 ? (u32)count : 1;
    }

#endif



    //- Mutex
    // A state of 0 is unlocked, 1 locked, and 2 locked with threads possibly asleep waiting for it. Unlocking from 2
    // wakes one of them. After Drepper's "Futexes Are Tricky".

    static constexpr u32 MutexUnlocked = 0;
    static constexpr u32 MutexLocked = 1;
    static constexpr u32 MutexContended = 2;

    static b8 MutexLockContended(Mutex* mutex, u32 timeout)
    {
        // The holder is often about to let go, and spinning is cheaper than sleeping.
        u32 state = AtomicU32Load(&mutex->state);
        for (u32 spin = 0; state == MutexLocked && spin < SpinCount; spin++)
        {
            SpinPause();
            state = AtomicU32Load(&mutex->state);
        }

        if (state == MutexUnlocked && AtomicU32CompareExchange(&mutex->state, &state, MutexLocked))
            return true;

        Timepoint start = WaitStart(timeout);
        while (true)
        {
            // Locking as contended is conservative: the unlock may wake a thread that isn't there.
            if (AtomicU32Exchange(&mutex->state, MutexContended) == MutexUnlocked)
                return true;

            // Giving up leaves the state contended, so the holder wakes another waiter in case this one took the wake.
            u32 remaining = WaitRemaining(start, timeout);
            if (remaining == 0)
                return false;

            AddressWait(&mutex->state, MutexContended, remaining);
        }
    }

    static void MutexClaim(Mutex* mutex)
    {
#if defined(TOOL_DEBUG_ASSERTS)
        AtomicU32Store(&mutex->owner, ThreadCurrentId());
#else
        (void)mutex;
#endif
    }

    void MutexLock(Mutex* mutex)
    {
        u32 expected = MutexUnlocked;
        if (!AtomicU32CompareExchange(&mutex->state, &expected, MutexLocked))
        {
            TOOL_DEBUG_ASSERT(AtomicU32Load(&mutex->owner) != ThreadCurrentId(), "Locking a Mutex this thread already holds.");
            MutexLockContended(mutex, WaitForever);
        }

        MutexClaim(mutex);
    }

    b8 MutexTryLock(Mutex* mutex, u32 timeout)
    {
        u32 expected = MutexUnlocked;
        if (!AtomicU32CompareExchange(&mutex->state, &expected, MutexLocked))
        {
            if (timeout == 0 || !MutexLockContended(mutex, timeout))
                return false;
        }

        MutexClaim(mutex);
        return true;
    }

    void MutexUnlock(Mutex* mutex)
    {
#if defined(TOOL_DEBUG_ASSERTS)
        TOOL_DEBUG_ASSERT(AtomicU32Load(&mutex->owner) == ThreadCurrentId(), "Unlocking a Mutex this thread doesn't hold.");
        AtomicU32Store(&mutex->owner, 0);
#endif

        if (AtomicU32Exchange(&mutex->state, MutexUnlocked) == MutexContended)
            AddressWakeOne(&mutex->state);
    }



    //- Condition
    // Waiters sleep on a counter that every wake bumps, so a wake between unlocking and sleeping isn't missed.

    void ConditionWait(Condition* condition, Mutex* mutex)
    {
        ConditionTryWait(condition, mutex, WaitForever);
    }

    b8 ConditionTryWait(Condition* condition, Mutex* mutex, u32 timeout)
    {
        u32 sequence = AtomicU32Load(&condition->sequence);

        MutexUnlock(mutex);
        b8 woken = AddressWait(&condition->sequence, sequence, timeout);
        MutexLock(mutex);

        return woken;
    }

    void ConditionWakeOne(Condition* condition)
    {
        AtomicU32Add(&condition->sequence, 1);
        AddressWakeOne(&condition->sequence);
    }

    void ConditionWakeAll(Condition* condition)
    {
        AtomicU32Add(&condition->sequence, 1);
        AddressWakeAll(&condition->sequence);
    }



    //- RWLock
    // The state's low 30 bits count readers, or are all set while write locked. The top two flag sleeping readers and
    // writers. Readers sleep on the state, writers on writerWake, which every writer wake bumps. After the futex
    // read-write lock in Rust's standard library.

    static constexpr u32 RWLockReadLocked = 1;
    static constexpr u32 RWLockMask = (1u << 30) - 1;
    static constexpr u32 RWLockWriteLocked = RWLockMask;
    static constexpr u32 RWLockMaxReaders = RWLockMask - 1;
    static constexpr u32 RWLockReadersWaiting = 1u << 30;
    static constexpr u32 RWLockWritersWaiting = 1u << 31;

    static inline b8 RWLockIsUnlocked(u32 state)
    {
        return (state & RWLockMask) == 0;
    }

    static inline b8 RWLockIsWriteLocked(u32 state)
    {
        return (state & RWLockMask) == RWLockWriteLocked;
    }

    // Readers queue behind anyone already waiting, so writers aren't starved.
    static inline b8 RWLockIsReadLockable(u32 state)
    {
        return (state & RWLockMask) < RWLockMaxReaders && (state & (RWLockReadersWaiting | RWLockWritersWaiting)) == 0;
    }

    static u32 RWLockSpinRead(RWLock* lock)
    {
        u32 state = AtomicU32Load(&lock->state);
        for (u32 spin = 0; spin < SpinCount && RWLockIsWriteLocked(state) && (state & (RWLockReadersWaiting | RWLockWritersWaiting)) == 0; spin++)
        {
            SpinPause();
            state = AtomicU32Load(&lock->state);
        }

        return state;
    }

    static u32 RWLockSpinWrite(RWLock* lock)
    {
        u32 state = AtomicU32Load(&lock->state);
        for (u32 spin = 0; spin < SpinCount && !RWLockIsUnlocked(state) && (state & RWLockWritersWaiting) == 0; spin++)
        {
            SpinPause();
            state = AtomicU32Load(&lock->state);
        }

        return state;
    }

    static b8 RWLockWakeWriter(RWLock* lock)
    {
        AtomicU32Add(&lock->writerWake, 1);
        return AddressWakeOne(&lock->writerWake);
    }

    // Called with the lock unlocked and someone waiting. Wakes one writer if any wait, otherwise every reader. If the
    // lock is taken meanwhile, its holder wakes the waiters when it unlocks.
    static void RWLockWake(RWLock* lock, u32 state)
    {
        if (state == RWLockWritersWaiting)
        {
            if (AtomicU32CompareExchange(&lock->state, &state, 0))
            {
                RWLockWakeWriter(lock);
                return;
            }
        }

        // Readers keep waiting for the writer, unless no writer was asleep to take the wake.
        if (state == (RWLockReadersWaiting | RWLockWritersWaiting))
        {
            if (!AtomicU32CompareExchange(&lock->state, &state, RWLockReadersWaiting))
                return;

            if (RWLockWakeWriter(lock))
                return;

            state = RWLockReadersWaiting;
        }

        if (state == RWLockReadersWaiting)
        {
            if (AtomicU32CompareExchange(&lock->state, &state, 0))
                AddressWakeAll(&lock->state);
        }
    }

    static b8 RWLockReadContended(RWLock* lock, u32 timeout)
    {
        Timepoint start = WaitStart(timeout);
        u32 state = RWLockSpinRead(lock);

        while (true)
        {
            if (RWLockIsReadLockable(state))
            {
                if (AtomicU32CompareExchange(&lock->state, &state, state + RWLockReadLocked))
                    return true;

                continue;
            }

            TOOL_ASSERT((state & RWLockMask) != RWLockMaxReaders, "Too many readers on an RWLock.");

            if ((state & RWLockReadersWaiting) == 0)
            {
                if (!AtomicU32CompareExchange(&lock->state, &state, state | RWLockReadersWaiting))
                    continue;

                state |= RWLockReadersWaiting;
            }

            // Readers are woken all at once, so a reader giving up can't have taken a wake from another.
            u32 remaining = WaitRemaining(start, timeout);
            if (remaining == 0)
                return false;

            AddressWait(&lock->state, state, remaining);
            state = RWLockSpinRead(lock);
        }
    }

    static b8 RWLockWriteContended(RWLock* lock, u32 timeout)
    {
        Timepoint start = WaitStart(timeout);
        u32 state = RWLockSpinWrite(lock);

        // A writer woken with others still asleep keeps their flag set when it locks, as the wake cleared it.
        u32 otherWritersWaiting = 0;

        while (true)
        {
            if (RWLockIsUnlocked(state))
            {
                if (AtomicU32CompareExchange(&lock->state, &state, state | RWLockWriteLocked | otherWritersWaiting))
                    return true;

                continue;
            }

            if ((state & RWLockWritersWaiting) == 0)
            {
                if (!AtomicU32CompareExchange(&lock->state, &state, state | RWLockWritersWaiting))
                    continue;
            }

            otherWritersWaiting = RWLockWritersWaiting;

            // Read the wake counter before checking the state again, so a wake in between isn't missed.
            u32 wake = AtomicU32Load(&lock->writerWake);
            state = AtomicU32Load(&lock->state);
            if (RWLockIsUnlocked(state) || (state & RWLockWritersWaiting) == 0)
                continue;

            u32 remaining = WaitRemaining(start, timeout);
            if (remaining == 0)
            {
                // This writer may have taken the wake meant to pass the lock on, so it passes one on: to another
                // writer, and to the readers if the lock is free with them waiting.
                RWLockWakeWriter(lock);
                state = AtomicU32Load(&lock->state);
                if (RWLockIsUnlocked(state) && (state & (RWLockReadersWaiting | RWLockWritersWaiting)) != 0)
                    RWLockWake(lock, state);

                return false;
            }

            AddressWait(&lock->writerWake, wake, remaining);
            state = RWLockSpinWrite(lock);
        }
    }

    void RWLockLockRead(RWLock* lock)
    {
        RWLockTryLockRead(lock, WaitForever);
    }

    b8 RWLockTryLockRead(RWLock* lock, u32 timeout)
    {
        u32 state = AtomicU32Load(&lock->state);
        while (RWLockIsReadLockable(state))
        {
            if (AtomicU32CompareExchange(&lock->state, &state, state + RWLockReadLocked))
                return true;
        }

        return timeout != 0 && RWLockReadContended(lock, timeout);
    }

    void RWLockUnlockRead(RWLock* lock)
    {
        u32 previous = AtomicU32Subtract(&lock->state, RWLockReadLocked);
        TOOL_DEBUG_ASSERT(!RWLockIsUnlocked(previous) && !RWLockIsWriteLocked(previous), "Read unlocking an RWLock that isn't read locked.");
        u32 state = previous - RWLockReadLocked;

        // Readers only wait on a read-locked lock behind a waiting writer, which wakes them when it's done.
        if (RWLockIsUnlocked(state) && (state & RWLockWritersWaiting) != 0)
            RWLockWake(lock, state);
    }

    void RWLockLockWrite(RWLock* lock)
    {
        RWLockTryLockWrite(lock, WaitForever);
    }

    b8 RWLockTryLockWrite(RWLock* lock, u32 timeout)
    {
        u32 state = AtomicU32Load(&lock->state);
        while (RWLockIsUnlocked(state))
        {
            if (AtomicU32CompareExchange(&lock->state, &state, state | RWLockWriteLocked))
                return true;
        }

        return timeout != 0 && RWLockWriteContended(lock, timeout);
    }

    void RWLockUnlockWrite(RWLock* lock)
    {
        u32 state = AtomicU32Subtract(&lock->state, RWLockWriteLocked) - RWLockWriteLocked;
        TOOL_DEBUG_ASSERT(RWLockIsUnlocked(state), "Write unlocking an RWLock that isn't write locked.");

        if ((state & (RWLockReadersWaiting | RWLockWritersWaiting)) != 0)
            RWLockWake(lock, state);
    }



    //- Semaphore
    // Posts only make the system call when someone may be asleep.

    void SemaphorePost(Semaphore* semaphore, u32 count)
    {
        AtomicU32Add(&semaphore->count, count);

        if (AtomicU32Load(&semaphore->waiters) != 0)
        {
            if (count == 1)
                AddressWakeOne(&semaphore->count);
            else
                AddressWakeAll(&semaphore->count);
        }
    }

    void SemaphoreWait(Semaphore* semaphore)
    {
        SemaphoreTryWait(semaphore, WaitForever);
    }

    b8 SemaphoreTryWait(Semaphore* semaphore, u32 timeout)
    {
        Timepoint start = WaitStart(timeout);

        while (true)
        {
            u32 count = AtomicU32Load(&semaphore->count);
            while (count != 0)
            {
                if (AtomicU32CompareExchange(&semaphore->count, &count, count - 1))
                    return true;
            }

            // A woken waiter always tries for the count before giving up, so it can't strand a post.
            u32 remaining = WaitRemaining(start, timeout);
            if (remaining == 0)
                return false;

            AtomicU32Add(&semaphore->waiters, 1);
            AddressWait(&semaphore->count, 0, remaining);
            AtomicU32Subtract(&semaphore->waiters, 1);
        }
    }



    //- Barrier
    // The last thread to arrive resets the count, then starts the next generation, which releases the rest.

    void BarrierInit(Barrier* barrier, u32 count)
    {
        TOOL_ASSERT(count > 0, "A Barrier needs at least one thread.");

        barrier->count = count;
        AtomicU32Store(&barrier->arrived, 0);
        AtomicU32Store(&barrier->generation, 0);
    }

    b8 BarrierWait(Barrier* barrier)
    {
        u32 generation = AtomicU32Load(&barrier->generation);

        if (AtomicU32Add(&barrier->arrived, 1) + 1 == barrier->count)
        {
            AtomicU32Store(&barrier->arrived, 0);
            AtomicU32Add(&barrier->generation, 1);
            AddressWakeAll(&barrier->generation);
            return true;
        }

        while (AtomicU32Load(&barrier->generation) == generation)
            AddressWait(&barrier->generation, generation, WaitForever);

        return false;
    }
}
