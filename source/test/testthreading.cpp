#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



static constexpr u32 workerCount = 8;

static void RunWorkers(ThreadFunction function, void* data, u32 count = workerCount)
{
	Thread threads[16];
	for (u32 i = 0; i < count; i++)
	{
		threads[i] = ThreadCreate(function, data);
		TOOL_ASSERT(threads[i] != 0);
	}

	for (u32 i = 0; i < count; i++)
		ThreadJoin(threads[i]);
}

static i64 MillisecondsSince(Timepoint start)
{
	return NanosecondsSince(start) / 1000000;
}

// A small per-thread generator for timeouts and choices.
static u32 Next(u32* state)
{
	*state = *state * 1664525u + 1013904223u;
	return *state >> 16;
}

static AtomicU32 seedCounter = {};

static u32 Seed()
{
	return AtomicU32Add(&seedCounter, 0x9E3779B9u) | 1;
}

//~ Atomics

static void TestAtomicOperations()
{
	AtomicU32 u = {};
	TOOL_ASSERT(AtomicU32Load(&u) == 0);
	AtomicU32Store(&u, 5);
	TOOL_ASSERT(AtomicU32Add(&u, 3) == 5 && AtomicU32Load(&u) == 8);
	TOOL_ASSERT(AtomicU32Subtract(&u, 10) == 8 && AtomicU32Load(&u) == 0xFFFFFFFEu);
	TOOL_ASSERT(AtomicU32And(&u, 0xF0) == 0xFFFFFFFEu && AtomicU32Load(&u) == 0xF0);
	TOOL_ASSERT(AtomicU32Or(&u, 0x0F) == 0xF0 && AtomicU32Xor(&u, 0xFF) == 0xFF && AtomicU32Load(&u) == 0);
	TOOL_ASSERT(AtomicU32Exchange(&u, 7) == 0);
	u32 expected = 6;
	TOOL_ASSERT(!AtomicU32CompareExchange(&u, &expected, 9) && expected == 7);
	TOOL_ASSERT(AtomicU32CompareExchange(&u, &expected, 9) && AtomicU32Load(&u) == 9);

	// 64-bit values have to survive whole, high half included.
	AtomicU64 wide = {};
	AtomicU64Store(&wide, 1ull << 40);
	TOOL_ASSERT(AtomicU64Add(&wide, 1ull << 33) == 1ull << 40 && AtomicU64Load(&wide) == (1ull << 40) + (1ull << 33));
	TOOL_ASSERT(AtomicU64Subtract(&wide, 1ull << 41) == (1ull << 40) + (1ull << 33));
	TOOL_ASSERT(AtomicU64Load(&wide) == (1ull << 33) - (1ull << 40));
	TOOL_ASSERT(AtomicU64And(&wide, 0xFFFFFFFF00000000ull) == (1ull << 33) - (1ull << 40));
	TOOL_ASSERT(AtomicU64Or(&wide, 1) == 0xFFFFFF0200000000ull && AtomicU64Xor(&wide, 0xFFFFFF0200000001ull) == 0xFFFFFF0200000001ull);
	TOOL_ASSERT(AtomicU64Exchange(&wide, 0x8000000000000001ull) == 0);
	u64 wideExpected = 1;
	TOOL_ASSERT(!AtomicU64CompareExchange(&wide, &wideExpected, 2) && wideExpected == 0x8000000000000001ull);
	TOOL_ASSERT(AtomicU64CompareExchange(&wide, &wideExpected, 2) && AtomicU64Load(&wide) == 2);

	// Signed arithmetic wraps.
	AtomicI32 i = {};
	AtomicI32Store(&i, I32_MIN);
	TOOL_ASSERT(AtomicI32Subtract(&i, 1) == I32_MIN && AtomicI32Load(&i) == I32_MAX);
	TOOL_ASSERT(AtomicI32Add(&i, 1) == I32_MAX && AtomicI32Load(&i) == I32_MIN);
	AtomicI32Store(&i, 0);
	TOOL_ASSERT(AtomicI32Subtract(&i, I32_MIN) == 0 && AtomicI32Load(&i) == I32_MIN);
	TOOL_ASSERT(AtomicI32Add(&i, -5) == I32_MIN && AtomicI32Load(&i) == I32_MAX - 4);
	i32 signedExpected = 0;
	TOOL_ASSERT(!AtomicI32CompareExchange(&i, &signedExpected, -1) && signedExpected == I32_MAX - 4);
	TOOL_ASSERT(AtomicI32CompareExchange(&i, &signedExpected, -1) && AtomicI32Exchange(&i, 3) == -1);
	TOOL_ASSERT(AtomicI32And(&i, 2) == 3 && AtomicI32Or(&i, -4) == 2 && AtomicI32Xor(&i, -1) == -2 && AtomicI32Load(&i) == 1);

	AtomicI64 l = {};
	AtomicI64Store(&l, I64_MIN);
	TOOL_ASSERT(AtomicI64Subtract(&l, 1) == I64_MIN && AtomicI64Load(&l) == I64_MAX);
	TOOL_ASSERT(AtomicI64Add(&l, 2) == I64_MAX && AtomicI64Load(&l) == I64_MIN + 1);
	TOOL_ASSERT(AtomicI64Exchange(&l, -7) == I64_MIN + 1);
	i64 longExpected = -7;
	TOOL_ASSERT(AtomicI64CompareExchange(&l, &longExpected, 7) && AtomicI64And(&l, 3) == 7 && AtomicI64Or(&l, 4) == 3);
	TOOL_ASSERT(AtomicI64Xor(&l, 1) == 7 && AtomicI64Load(&l) == 6);

	i32 a = 0;
	i32 b = 0;
	AtomicPointer pointer = {};
	TOOL_ASSERT(AtomicPointerLoad(&pointer) == nullptr);
	AtomicPointerStore(&pointer, &a);
	TOOL_ASSERT(AtomicPointerExchange(&pointer, &b) == &a);
	void* pointerExpected = &a;
	TOOL_ASSERT(!AtomicPointerCompareExchange(&pointer, &pointerExpected, nullptr) && pointerExpected == &b);
	TOOL_ASSERT(AtomicPointerCompareExchange(&pointer, &pointerExpected, nullptr) && AtomicPointerLoad(&pointer) == nullptr);
}

struct CounterTest
{
	AtomicU64 total;
	AtomicI64 balance;
	AtomicU32 swapped;
};

static void CounterWork(void* data)
{
	CounterTest* test = (CounterTest*)data;
	for (u32 i = 0; i < 100000; i++)
	{
		AtomicU64Add(&test->total, 3);
		AtomicI64Add(&test->balance, 5);
		AtomicI64Subtract(&test->balance, 5);

		// Increments through compare-exchange retries
		u32 seen = AtomicU32Load(&test->swapped);
		while (!AtomicU32CompareExchange(&test->swapped, &seen, seen + 1)) {}
	}
}

static void TestAtomicContention()
{
	CounterTest test = {};
	RunWorkers(CounterWork, &test);
	TOOL_ASSERT(AtomicU64Load(&test.total) == 3ull * 100000 * workerCount);
	TOOL_ASSERT(AtomicI64Load(&test.balance) == 0);
	TOOL_ASSERT(AtomicU32Load(&test.swapped) == 100000 * workerCount);
}

//~ Threads

struct ThreadTest
{
	AtomicU32 ids[workerCount];
	AtomicU32 next;
	AtomicU32 detachedDone;
};

static void RecordId(void* data)
{
	ThreadTest* test = (ThreadTest*)data;
	ThreadSetName("TOOL test worker with a long name");
	AtomicU32Store(&test->ids[AtomicU32Add(&test->next, 1)], ThreadCurrentId());
}

static void SleepThenEnd(void* data)
{
	ThreadSleep(*(u32*)data);
}

static void SignalDone(void* data)
{
	ThreadTest* test = (ThreadTest*)data;
	AtomicU32Store(&test->detachedDone, 1);
	AtomicU32WakeAll(&test->detachedDone);
}

static void TestThreads()
{
	TOOL_ASSERT(ProcessorCount() >= 1);

	// IDs are nonzero and distinct, the main thread's included.
	ThreadTest test = {};
	RunWorkers(RecordId, &test);
	for (u32 i = 0; i < workerCount; i++)
	{
		u32 id = AtomicU32Load(&test.ids[i]);
		TOOL_ASSERT(id != 0 && id != ThreadCurrentId());
		for (u32 j = 0; j < i; j++)
			TOOL_ASSERT(id != AtomicU32Load(&test.ids[j]));
	}

	// Joining with a timeout gives up while the thread runs, and joins once it's done.
	u32 sleep = 100;
	Thread sleeper = ThreadCreate(SleepThenEnd, &sleep);
	TOOL_ASSERT(!ThreadTryJoin(sleeper));
	Timepoint start = TimepointNow();
	TOOL_ASSERT(!ThreadTryJoin(sleeper, 20));
	TOOL_ASSERT(MillisecondsSince(start) >= 15);
	TOOL_ASSERT(ThreadTryJoin(sleeper, 5000));

	Thread detached = ThreadCreate(SignalDone, &test);
	ThreadDetach(detached);
	while (AtomicU32Load(&test.detachedDone) == 0)
		AtomicU32Wait(&test.detachedDone, 0);

	start = TimepointNow();
	ThreadSleep(30);
	TOOL_ASSERT(MillisecondsSince(start) >= 25);
	ThreadYield();

	// Waiting on an atomic returns at once if it already changed, and times out if it doesn't.
	AtomicU32 flag = {};
	TOOL_ASSERT(AtomicU32TryWait(&flag, 1, 1000));
	start = TimepointNow();
	TOOL_ASSERT(!AtomicU32TryWait(&flag, 0, 20));
	TOOL_ASSERT(MillisecondsSince(start) >= 15);
}

//~ Mutex

struct MutexTest
{
	Mutex mutex;
	u64 counter;
	u64 entries;
	AtomicU32 inside;
	AtomicU32 index;
};

static void MutexEnter(MutexTest* test)
{
	TOOL_ASSERT(AtomicU32Exchange(&test->inside, 1) == 0);
	test->counter++;
	AtomicU32Store(&test->inside, 0);
}

static void MutexWork(void* data)
{
	MutexTest* test = (MutexTest*)data;
	for (u32 i = 0; i < 50000; i++)
	{
		MutexLock(&test->mutex);
		MutexEnter(test);
		MutexUnlock(&test->mutex);
	}
}

// Half block, half give up after short timeouts. If a waiter that gives up strands a wake, the blocking ones hang.
static void MutexMixedWork(void* data)
{
	MutexTest* test = (MutexTest*)data;
	u32 random = Seed();
	b8 blocking = AtomicU32Add(&test->index, 1) % 2;

	for (u32 i = 0; i < 20000; i++)
	{
		b8 locked = true;
		if (blocking)
			MutexLock(&test->mutex);
		else
			locked = MutexTryLock(&test->mutex, Next(&random) % 3);

		if (locked)
		{
			MutexEnter(test);
			test->entries++;
			for (u32 spin = Next(&random) % 64; spin > 0; spin--)
				AtomicU32Load(&test->inside);
			MutexUnlock(&test->mutex);
		}
	}
}

static void MutexHolder(void* data)
{
	MutexTest* test = (MutexTest*)data;
	TOOL_ASSERT(!MutexTryLock(&test->mutex));

	Timepoint start = TimepointNow();
	TOOL_ASSERT(!MutexTryLock(&test->mutex, 30));
	TOOL_ASSERT(MillisecondsSince(start) >= 25);

	AtomicU32Store(&test->inside, 1);
	AtomicU32WakeAll(&test->inside);
	TOOL_ASSERT(MutexTryLock(&test->mutex, 5000));
	MutexUnlock(&test->mutex);
}

static void TestMutex()
{
	MutexTest test = {};
	RunWorkers(MutexWork, &test);
	TOOL_ASSERT(test.counter == 50000ull * workerCount);

	MutexTest mixed = {};
	RunWorkers(MutexMixedWork, &mixed);
	TOOL_ASSERT(mixed.counter == mixed.entries && mixed.entries > 0);

	// Timeouts expire while another thread holds the lock, and succeed once it lets go.
	MutexTest timed = {};
	MutexLock(&timed.mutex);
	Thread thread = ThreadCreate(MutexHolder, &timed);
	while (AtomicU32Load(&timed.inside) == 0)
		AtomicU32Wait(&timed.inside, 0);
	MutexUnlock(&timed.mutex);
	ThreadJoin(thread);
	TOOL_ASSERT(MutexTryLock(&timed.mutex));
	MutexUnlock(&timed.mutex);
}

//~ Condition

struct QueueTest
{
	Mutex mutex;
	Condition notEmpty;
	Condition notFull;

	u32 items[16];
	u32 head;
	u32 count;

	u32 taken;
	u64 sum;
	AtomicU32 index;
};

static constexpr u32 queueProducers = 4;
static constexpr u32 queueItems = 20000; // Per producer
static constexpr u32 queueTotal = queueProducers * queueItems;

static void QueueProduce(void* data)
{
	QueueTest* test = (QueueTest*)data;
	for (u32 value = 1; value <= queueItems; value++)
	{
		MutexLock(&test->mutex);
		while (test->count == 16)
			ConditionWait(&test->notFull, &test->mutex);

		test->items[(test->head + test->count) % 16] = value;
		test->count++;
		ConditionWakeOne(&test->notEmpty);
		MutexUnlock(&test->mutex);
	}
}

// Every other consumer waits with short timeouts, which must not lose items or wakes.
static void QueueConsume(void* data)
{
	QueueTest* test = (QueueTest*)data;
	b8 timed = AtomicU32Add(&test->index, 1) % 2;

	while (true)
	{
		MutexLock(&test->mutex);
		while (test->count == 0 && test->taken < queueTotal)
		{
			if (timed)
				ConditionTryWait(&test->notEmpty, &test->mutex, 1);
			else
				ConditionWait(&test->notEmpty, &test->mutex);
		}

		if (test->taken == queueTotal)
		{
			MutexUnlock(&test->mutex);
			return;
		}

		test->sum += test->items[test->head];
		test->head = (test->head + 1) % 16;
		test->count--;
		test->taken++;

		if (test->taken == queueTotal)
			ConditionWakeAll(&test->notEmpty);
		ConditionWakeOne(&test->notFull);
		MutexUnlock(&test->mutex);
	}
}

static void QueueRun(void* data)
{
	QueueTest* test = (QueueTest*)data;
	if (AtomicU32Add(&test->index, 1) < queueProducers)
		QueueProduce(test);
	else
		QueueConsume(test);
}

static void TestCondition()
{
	QueueTest test = {};
	RunWorkers(QueueRun, &test);
	TOOL_ASSERT(test.taken == queueTotal && test.count == 0);
	TOOL_ASSERT(test.sum == (u64)queueProducers * queueItems * (queueItems + 1) / 2);

	// A timed wait without a wake times out, and holds the mutex again either way.
	Mutex mutex = {};
	Condition condition = {};
	MutexLock(&mutex);
	Timepoint start = TimepointNow();
	TOOL_ASSERT(!ConditionTryWait(&condition, &mutex, 30));
	TOOL_ASSERT(MillisecondsSince(start) >= 25);
	TOOL_ASSERT(!MutexTryLock(&mutex));
	MutexUnlock(&mutex);
}

//~ RWLock

struct RWLockTest
{
	RWLock lock;
	u64 a;
	u64 b;
	u64 writes;
	AtomicU32 readers;
	AtomicU32 writers;
	AtomicU32 index;
	AtomicU32 waiting;
};

static void RWLockWork(void* data)
{
	RWLockTest* test = (RWLockTest*)data;
	u32 index = AtomicU32Add(&test->index, 1);
	u32 random = Seed();
	b8 writer = index < 3;
	b8 timed = index % 2 == 1;

	for (u32 i = 0; i < (writer ? 10000u : 30000u); i++)
	{
		if (writer)
		{
			if (!timed)
				RWLockLockWrite(&test->lock);
			else if (!RWLockTryLockWrite(&test->lock, Next(&random) % 3))
				continue;

			TOOL_ASSERT(AtomicU32Exchange(&test->writers, 1) == 0 && AtomicU32Load(&test->readers) == 0);
			test->a++;
			test->b++;
			test->writes++;
			AtomicU32Store(&test->writers, 0);
			RWLockUnlockWrite(&test->lock);
		}
		else
		{
			if (!timed)
				RWLockLockRead(&test->lock);
			else if (!RWLockTryLockRead(&test->lock, Next(&random) % 3))
				continue;

			AtomicU32Add(&test->readers, 1);
			TOOL_ASSERT(AtomicU32Load(&test->writers) == 0 && test->a == test->b);
			AtomicU32Subtract(&test->readers, 1);
			RWLockUnlockRead(&test->lock);
		}
	}
}

static void RWLockWaitingWriter(void* data)
{
	RWLockTest* test = (RWLockTest*)data;
	AtomicU32Store(&test->waiting, 1);
	RWLockLockWrite(&test->lock);
	test->a = 1;
	RWLockUnlockWrite(&test->lock);
}

static void TestRWLock()
{
	RWLockTest test = {};
	RunWorkers(RWLockWork, &test);
	TOOL_ASSERT(test.a == test.writes && test.b == test.writes && test.writes > 0);

	// Readers share, writers don't.
	RWLock lock = {};
	RWLockLockRead(&lock);
	TOOL_ASSERT(RWLockTryLockRead(&lock));
	TOOL_ASSERT(!RWLockTryLockWrite(&lock));
	Timepoint start = TimepointNow();
	TOOL_ASSERT(!RWLockTryLockWrite(&lock, 30));
	TOOL_ASSERT(MillisecondsSince(start) >= 25);
	RWLockUnlockRead(&lock);
	RWLockUnlockRead(&lock);

	TOOL_ASSERT(RWLockTryLockWrite(&lock));
	TOOL_ASSERT(!RWLockTryLockRead(&lock));
	start = TimepointNow();
	TOOL_ASSERT(!RWLockTryLockRead(&lock, 30));
	TOOL_ASSERT(MillisecondsSince(start) >= 25);
	RWLockUnlockWrite(&lock);

	// A writer waiting behind a reader holds back new readers.
	RWLockTest preference = {};
	RWLockLockRead(&preference.lock);
	Thread writer = ThreadCreate(RWLockWaitingWriter, &preference);
	while (AtomicU32Load(&preference.waiting) == 0)
		ThreadYield();
	ThreadSleep(50);
	TOOL_ASSERT(!RWLockTryLockRead(&preference.lock));
	RWLockUnlockRead(&preference.lock);
	ThreadJoin(writer);
	RWLockLockRead(&preference.lock);
	TOOL_ASSERT(preference.a == 1);
	RWLockUnlockRead(&preference.lock);
}

//~ Semaphore

struct SemaphoreTest
{
	Semaphore semaphore;
	AtomicU32 index;
	AtomicU32 taken;
};

static constexpr u32 semaphoreTokens = 20000; // Per poster

static void SemaphoreWork(void* data)
{
	SemaphoreTest* test = (SemaphoreTest*)data;
	u32 index = AtomicU32Add(&test->index, 1);
	u32 random = Seed();

	if (index < workerCount / 2)
	{
		for (u32 i = 0; i < semaphoreTokens; i += 2)
		{
			SemaphorePost(&test->semaphore);
			SemaphorePost(&test->semaphore, 1);
		}
		return;
	}

	// Takers alternate blocking and timed waits until every token is gone.
	for (u32 i = 0; i < semaphoreTokens; i++)
	{
		if (index % 2 == 0)
			SemaphoreWait(&test->semaphore);
		else
			while (!SemaphoreTryWait(&test->semaphore, Next(&random) % 3)) {}

		AtomicU32Add(&test->taken, 1);
	}
}

static void TestSemaphore()
{
	Semaphore semaphore = {};
	TOOL_ASSERT(!SemaphoreTryWait(&semaphore));
	SemaphorePost(&semaphore, 3);
	TOOL_ASSERT(SemaphoreTryWait(&semaphore) && SemaphoreTryWait(&semaphore) && SemaphoreTryWait(&semaphore));
	TOOL_ASSERT(!SemaphoreTryWait(&semaphore));
	Timepoint start = TimepointNow();
	TOOL_ASSERT(!SemaphoreTryWait(&semaphore, 30));
	TOOL_ASSERT(MillisecondsSince(start) >= 25);

	SemaphoreTest test = {};
	RunWorkers(SemaphoreWork, &test);
	TOOL_ASSERT(AtomicU32Load(&test.taken) == semaphoreTokens * workerCount / 2);
	TOOL_ASSERT(!SemaphoreTryWait(&test.semaphore));
}

//~ Barrier

struct BarrierTest
{
	Barrier barrier;
	AtomicU32 arrivals;
	AtomicU32 serial;
};

static constexpr u32 barrierRounds = 2000;

static void BarrierWork(void* data)
{
	BarrierTest* test = (BarrierTest*)data;
	for (u32 round = 0; round < barrierRounds; round++)
	{
		AtomicU32Add(&test->arrivals, 1);
		if (BarrierWait(&test->barrier))
			AtomicU32Add(&test->serial, 1);

		// Everyone arrived for this round; nobody can have finished the next.
		u32 arrivals = AtomicU32Load(&test->arrivals);
		TOOL_ASSERT(arrivals >= (round + 1) * workerCount && arrivals < (round + 2) * workerCount);
	}
}

static void TestBarrier()
{
	BarrierTest test = {};
	BarrierInit(&test.barrier, workerCount);
	RunWorkers(BarrierWork, &test);
	TOOL_ASSERT(AtomicU32Load(&test.serial) == barrierRounds);
	TOOL_ASSERT(AtomicU32Load(&test.arrivals) == barrierRounds * workerCount);

	Barrier single = {};
	BarrierInit(&single, 1);
	TOOL_ASSERT(BarrierWait(&single) && BarrierWait(&single));
}



void TestThreading()
{
	TestAtomicOperations();
	TestAtomicContention();
	TestThreads();
	TestMutex();
	TestCondition();
	TestRWLock();
	TestSemaphore();
	TestBarrier();
	printf("Threading: passed\n");
}
