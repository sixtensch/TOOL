#ifndef _TOOL_ATOMIC_H
#define _TOOL_ATOMIC_H

#include "basics.h"



//~ Atomics
//
// Values shared between threads, read and changed only through these functions. Every operation is sequentially
// consistent: all threads see all atomic operations happen in one order, consistent with each thread's own order.
// Zero-initialized atomics hold 0. Each type has the same size and alignment on every platform, except that
// AtomicPointer is pointer sized.
//
// Waiting for an atomic to change, and waking its waiters, lives in threading.h.



namespace Tool
{
	//- Types

	struct alignas(4) AtomicU32
	{
		u32 value;
	};

	struct alignas(8) AtomicU64
	{
		u64 value;
	};

	struct alignas(4) AtomicI32
	{
		i32 value;
	};

	struct alignas(8) AtomicI64
	{
		i64 value;
	};

	struct alignas(sizeof(void*)) AtomicPointer
	{
		void* value;
	};

	static_assert(sizeof(AtomicU32) == 4 && alignof(AtomicU32) == 4, "AtomicU32 layout");
	static_assert(sizeof(AtomicU64) == 8 && alignof(AtomicU64) == 8, "AtomicU64 layout");
	static_assert(sizeof(AtomicI32) == 4 && alignof(AtomicI32) == 4, "AtomicI32 layout");
	static_assert(sizeof(AtomicI64) == 8 && alignof(AtomicI64) == 8, "AtomicI64 layout");
	static_assert(sizeof(AtomicPointer) == sizeof(void*) && alignof(AtomicPointer) == sizeof(void*), "AtomicPointer layout");



	//- Functions
	// Exchange, Add, Subtract, And, Or and Xor return the value from before the operation. Arithmetic wraps.
	// CompareExchange stores desired if the atomic holds expected and returns true. Otherwise it writes the value it
	// found to expected and returns false.

	//~ u32

	inline u32 AtomicU32Load(const AtomicU32* atomic);
	inline void AtomicU32Store(AtomicU32* atomic, u32 value);
	inline u32 AtomicU32Exchange(AtomicU32* atomic, u32 value);
	inline b8 AtomicU32CompareExchange(AtomicU32* atomic, u32* expected, u32 desired);
	inline u32 AtomicU32Add(AtomicU32* atomic, u32 value);
	inline u32 AtomicU32Subtract(AtomicU32* atomic, u32 value);
	inline u32 AtomicU32And(AtomicU32* atomic, u32 value);
	inline u32 AtomicU32Or(AtomicU32* atomic, u32 value);
	inline u32 AtomicU32Xor(AtomicU32* atomic, u32 value);

	//~ u64

	inline u64 AtomicU64Load(const AtomicU64* atomic);
	inline void AtomicU64Store(AtomicU64* atomic, u64 value);
	inline u64 AtomicU64Exchange(AtomicU64* atomic, u64 value);
	inline b8 AtomicU64CompareExchange(AtomicU64* atomic, u64* expected, u64 desired);
	inline u64 AtomicU64Add(AtomicU64* atomic, u64 value);
	inline u64 AtomicU64Subtract(AtomicU64* atomic, u64 value);
	inline u64 AtomicU64And(AtomicU64* atomic, u64 value);
	inline u64 AtomicU64Or(AtomicU64* atomic, u64 value);
	inline u64 AtomicU64Xor(AtomicU64* atomic, u64 value);

	//~ i32

	inline i32 AtomicI32Load(const AtomicI32* atomic);
	inline void AtomicI32Store(AtomicI32* atomic, i32 value);
	inline i32 AtomicI32Exchange(AtomicI32* atomic, i32 value);
	inline b8 AtomicI32CompareExchange(AtomicI32* atomic, i32* expected, i32 desired);
	inline i32 AtomicI32Add(AtomicI32* atomic, i32 value);
	inline i32 AtomicI32Subtract(AtomicI32* atomic, i32 value);
	inline i32 AtomicI32And(AtomicI32* atomic, i32 value);
	inline i32 AtomicI32Or(AtomicI32* atomic, i32 value);
	inline i32 AtomicI32Xor(AtomicI32* atomic, i32 value);

	//~ i64

	inline i64 AtomicI64Load(const AtomicI64* atomic);
	inline void AtomicI64Store(AtomicI64* atomic, i64 value);
	inline i64 AtomicI64Exchange(AtomicI64* atomic, i64 value);
	inline b8 AtomicI64CompareExchange(AtomicI64* atomic, i64* expected, i64 desired);
	inline i64 AtomicI64Add(AtomicI64* atomic, i64 value);
	inline i64 AtomicI64Subtract(AtomicI64* atomic, i64 value);
	inline i64 AtomicI64And(AtomicI64* atomic, i64 value);
	inline i64 AtomicI64Or(AtomicI64* atomic, i64 value);
	inline i64 AtomicI64Xor(AtomicI64* atomic, i64 value);

	//~ Pointer

	inline void* AtomicPointerLoad(const AtomicPointer* atomic);
	inline void AtomicPointerStore(AtomicPointer* atomic, void* value);
	inline void* AtomicPointerExchange(AtomicPointer* atomic, void* value);
	inline b8 AtomicPointerCompareExchange(AtomicPointer* atomic, void** expected, void* desired);
} //namespace Tool



//- Implementation
//
// GCC and Clang (clang-cl included) use their __atomic builtins. MSVC uses its Interlocked intrinsics, which are full
// barriers, with loads as plain volatile loads behind a barrier, as its own <atomic> does. 32-bit x86 has no 64-bit
// Interlocked arithmetic, so it loops on a compare-exchange instead.
//
// Every operation goes through one template per operation over the unsigned type of the same width.

#if defined(__clang__) || defined(__GNUC__)
#define TOOL_ATOMIC_BUILTIN 1
#elif defined(_MSC_VER)
// Declared as <intrin.h> does, to spare every includer that header.
extern "C" long _InterlockedExchange(long volatile* target, long value);
extern "C" long _InterlockedCompareExchange(long volatile* destination, long exchange, long comparand);
extern "C" long _InterlockedExchangeAdd(long volatile* addend, long value);
extern "C" long _InterlockedAnd(long volatile* value, long mask);
extern "C" long _InterlockedOr(long volatile* value, long mask);
extern "C" long _InterlockedXor(long volatile* value, long mask);
extern "C" __int64 _InterlockedCompareExchange64(__int64 volatile* destination, __int64 exchange, __int64 comparand);
extern "C" __int32 __iso_volatile_load32(const volatile __int32* location);
extern "C" __int64 __iso_volatile_load64(const volatile __int64* location);
extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_InterlockedExchange, _InterlockedCompareExchange, _InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedAnd, _InterlockedOr, _InterlockedXor, _InterlockedCompareExchange64)
#pragma intrinsic(__iso_volatile_load32, __iso_volatile_load64, _ReadWriteBarrier)
#if !defined(_M_IX86)
extern "C" __int64 _InterlockedExchange64(__int64 volatile* target, __int64 value);
extern "C" __int64 _InterlockedExchangeAdd64(__int64 volatile* addend, __int64 value);
extern "C" __int64 _InterlockedAnd64(__int64 volatile* value, __int64 mask);
extern "C" __int64 _InterlockedOr64(__int64 volatile* value, __int64 mask);
extern "C" __int64 _InterlockedXor64(__int64 volatile* value, __int64 mask);
#pragma intrinsic(_InterlockedExchange64, _InterlockedExchangeAdd64, _InterlockedAnd64, _InterlockedOr64, _InterlockedXor64)
#endif
#if defined(_M_ARM64) || defined(_M_ARM64EC)
extern "C" void __dmb(unsigned int type);
#pragma intrinsic(__dmb)
#endif
#else
#error "No atomics for this compiler"
#endif

namespace Tool
{
	//~ Width-generic operations

#if defined(TOOL_ATOMIC_BUILTIN)

	template<typename T> inline T AtomicRawLoad(const T* atomic) { return __atomic_load_n(atomic, __ATOMIC_SEQ_CST); }
	template<typename T> inline void AtomicRawStore(T* atomic, T value) { __atomic_store_n(atomic, value, __ATOMIC_SEQ_CST); }
	template<typename T> inline T AtomicRawExchange(T* atomic, T value) { return __atomic_exchange_n(atomic, value, __ATOMIC_SEQ_CST); }
	template<typename T> inline T AtomicRawAdd(T* atomic, T value) { return __atomic_fetch_add(atomic, value, __ATOMIC_SEQ_CST); }
	template<typename T> inline T AtomicRawAnd(T* atomic, T value) { return __atomic_fetch_and(atomic, value, __ATOMIC_SEQ_CST); }
	template<typename T> inline T AtomicRawOr(T* atomic, T value) { return __atomic_fetch_or(atomic, value, __ATOMIC_SEQ_CST); }
	template<typename T> inline T AtomicRawXor(T* atomic, T value) { return __atomic_fetch_xor(atomic, value, __ATOMIC_SEQ_CST); }

	template<typename T>
	inline b8 AtomicRawCompareExchange(T* atomic, T* expected, T desired)
	{
		return __atomic_compare_exchange_n(atomic, expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	}

#else

	// Orders a plain load after every earlier atomic operation and before every later one.
	inline void AtomicLoadBarrier()
	{
#if defined(_M_ARM64) || defined(_M_ARM64EC)
		__dmb(0xB); // Inner shareable, full
#else
		_ReadWriteBarrier(); // x86 never moves loads before earlier locked operations; only the compiler might
#endif
	}

	template<typename T>
	inline T AtomicRawCompareExchangeValue(T* atomic, T expected, T desired) // Returns the value found
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedCompareExchange((volatile long*)atomic, (long)desired, (long)expected);
		else
			return (T)_InterlockedCompareExchange64((volatile __int64*)atomic, (__int64)desired, (__int64)expected);
	}

	template<typename T>
	inline T AtomicRawLoad(const T* atomic)
	{
		T value;
		if constexpr (sizeof(T) == 4)
			value = (T)__iso_volatile_load32((const volatile __int32*)atomic);
		else
			value = (T)__iso_volatile_load64((const volatile __int64*)atomic);
		AtomicLoadBarrier();
		return value;
	}

	template<typename T>
	inline T AtomicRawExchange(T* atomic, T value)
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedExchange((volatile long*)atomic, (long)value);
		else
		{
#if defined(_M_IX86)
			T found = AtomicRawLoad(atomic);
			for (T seen; (seen = AtomicRawCompareExchangeValue(atomic, found, value)) != found; found = seen) {}
			return found;
#else
			return (T)_InterlockedExchange64((volatile __int64*)atomic, (__int64)value);
#endif
		}
	}

	template<typename T>
	inline void AtomicRawStore(T* atomic, T value)
	{
		AtomicRawExchange(atomic, value); // A locked exchange, so later loads can't pass the store
	}

	template<typename T>
	inline b8 AtomicRawCompareExchange(T* atomic, T* expected, T desired)
	{
		T found = AtomicRawCompareExchangeValue(atomic, *expected, desired);
		if (found == *expected)
			return true;
		*expected = found;
		return false;
	}

	// The 64-bit read-modify-write fallback for 32-bit x86: retries until no other thread changed the value between.
#define TOOL_ATOMIC_UPDATE_64(atomic, update) \
	T found = AtomicRawLoad(atomic); \
	for (T seen; (seen = AtomicRawCompareExchangeValue(atomic, found, (T)(update))) != found; found = seen) {} \
	return found

	template<typename T>
	inline T AtomicRawAdd(T* atomic, T value)
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedExchangeAdd((volatile long*)atomic, (long)value);
		else
		{
#if defined(_M_IX86)
			TOOL_ATOMIC_UPDATE_64(atomic, found + value);
#else
			return (T)_InterlockedExchangeAdd64((volatile __int64*)atomic, (__int64)value);
#endif
		}
	}

	template<typename T>
	inline T AtomicRawAnd(T* atomic, T value)
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedAnd((volatile long*)atomic, (long)value);
		else
		{
#if defined(_M_IX86)
			TOOL_ATOMIC_UPDATE_64(atomic, found & value);
#else
			return (T)_InterlockedAnd64((volatile __int64*)atomic, (__int64)value);
#endif
		}
	}

	template<typename T>
	inline T AtomicRawOr(T* atomic, T value)
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedOr((volatile long*)atomic, (long)value);
		else
		{
#if defined(_M_IX86)
			TOOL_ATOMIC_UPDATE_64(atomic, found | value);
#else
			return (T)_InterlockedOr64((volatile __int64*)atomic, (__int64)value);
#endif
		}
	}

	template<typename T>
	inline T AtomicRawXor(T* atomic, T value)
	{
		if constexpr (sizeof(T) == 4)
			return (T)_InterlockedXor((volatile long*)atomic, (long)value);
		else
		{
#if defined(_M_IX86)
			TOOL_ATOMIC_UPDATE_64(atomic, found ^ value);
#else
			return (T)_InterlockedXor64((volatile __int64*)atomic, (__int64)value);
#endif
		}
	}

#undef TOOL_ATOMIC_UPDATE_64

#endif

	//~ u32

	inline u32 AtomicU32Load(const AtomicU32* atomic) { return AtomicRawLoad(&atomic->value); }
	inline void AtomicU32Store(AtomicU32* atomic, u32 value) { AtomicRawStore(&atomic->value, value); }
	inline u32 AtomicU32Exchange(AtomicU32* atomic, u32 value) { return AtomicRawExchange(&atomic->value, value); }
	inline b8 AtomicU32CompareExchange(AtomicU32* atomic, u32* expected, u32 desired) { return AtomicRawCompareExchange(&atomic->value, expected, desired); }
	inline u32 AtomicU32Add(AtomicU32* atomic, u32 value) { return AtomicRawAdd(&atomic->value, value); }
	inline u32 AtomicU32Subtract(AtomicU32* atomic, u32 value) { return AtomicRawAdd(&atomic->value, 0u - value); }
	inline u32 AtomicU32And(AtomicU32* atomic, u32 value) { return AtomicRawAnd(&atomic->value, value); }
	inline u32 AtomicU32Or(AtomicU32* atomic, u32 value) { return AtomicRawOr(&atomic->value, value); }
	inline u32 AtomicU32Xor(AtomicU32* atomic, u32 value) { return AtomicRawXor(&atomic->value, value); }

	//~ u64

	inline u64 AtomicU64Load(const AtomicU64* atomic) { return AtomicRawLoad(&atomic->value); }
	inline void AtomicU64Store(AtomicU64* atomic, u64 value) { AtomicRawStore(&atomic->value, value); }
	inline u64 AtomicU64Exchange(AtomicU64* atomic, u64 value) { return AtomicRawExchange(&atomic->value, value); }
	inline b8 AtomicU64CompareExchange(AtomicU64* atomic, u64* expected, u64 desired) { return AtomicRawCompareExchange(&atomic->value, expected, desired); }
	inline u64 AtomicU64Add(AtomicU64* atomic, u64 value) { return AtomicRawAdd(&atomic->value, value); }
	inline u64 AtomicU64Subtract(AtomicU64* atomic, u64 value) { return AtomicRawAdd(&atomic->value, 0ull - value); }
	inline u64 AtomicU64And(AtomicU64* atomic, u64 value) { return AtomicRawAnd(&atomic->value, value); }
	inline u64 AtomicU64Or(AtomicU64* atomic, u64 value) { return AtomicRawOr(&atomic->value, value); }
	inline u64 AtomicU64Xor(AtomicU64* atomic, u64 value) { return AtomicRawXor(&atomic->value, value); }

	//~ i32
	// Through the unsigned type, whose arithmetic wraps by definition.

	inline i32 AtomicI32Load(const AtomicI32* atomic) { return (i32)AtomicRawLoad((const u32*)&atomic->value); }
	inline void AtomicI32Store(AtomicI32* atomic, i32 value) { AtomicRawStore((u32*)&atomic->value, (u32)value); }
	inline i32 AtomicI32Exchange(AtomicI32* atomic, i32 value) { return (i32)AtomicRawExchange((u32*)&atomic->value, (u32)value); }
	inline b8 AtomicI32CompareExchange(AtomicI32* atomic, i32* expected, i32 desired) { return AtomicRawCompareExchange((u32*)&atomic->value, (u32*)expected, (u32)desired); }
	inline i32 AtomicI32Add(AtomicI32* atomic, i32 value) { return (i32)AtomicRawAdd((u32*)&atomic->value, (u32)value); }
	inline i32 AtomicI32Subtract(AtomicI32* atomic, i32 value) { return (i32)AtomicRawAdd((u32*)&atomic->value, 0u - (u32)value); }
	inline i32 AtomicI32And(AtomicI32* atomic, i32 value) { return (i32)AtomicRawAnd((u32*)&atomic->value, (u32)value); }
	inline i32 AtomicI32Or(AtomicI32* atomic, i32 value) { return (i32)AtomicRawOr((u32*)&atomic->value, (u32)value); }
	inline i32 AtomicI32Xor(AtomicI32* atomic, i32 value) { return (i32)AtomicRawXor((u32*)&atomic->value, (u32)value); }

	//~ i64

	inline i64 AtomicI64Load(const AtomicI64* atomic) { return (i64)AtomicRawLoad((const u64*)&atomic->value); }
	inline void AtomicI64Store(AtomicI64* atomic, i64 value) { AtomicRawStore((u64*)&atomic->value, (u64)value); }
	inline i64 AtomicI64Exchange(AtomicI64* atomic, i64 value) { return (i64)AtomicRawExchange((u64*)&atomic->value, (u64)value); }
	inline b8 AtomicI64CompareExchange(AtomicI64* atomic, i64* expected, i64 desired) { return AtomicRawCompareExchange((u64*)&atomic->value, (u64*)expected, (u64)desired); }
	inline i64 AtomicI64Add(AtomicI64* atomic, i64 value) { return (i64)AtomicRawAdd((u64*)&atomic->value, (u64)value); }
	inline i64 AtomicI64Subtract(AtomicI64* atomic, i64 value) { return (i64)AtomicRawAdd((u64*)&atomic->value, 0ull - (u64)value); }
	inline i64 AtomicI64And(AtomicI64* atomic, i64 value) { return (i64)AtomicRawAnd((u64*)&atomic->value, (u64)value); }
	inline i64 AtomicI64Or(AtomicI64* atomic, i64 value) { return (i64)AtomicRawOr((u64*)&atomic->value, (u64)value); }
	inline i64 AtomicI64Xor(AtomicI64* atomic, i64 value) { return (i64)AtomicRawXor((u64*)&atomic->value, (u64)value); }

	//~ Pointer
	// Through the unsigned type of pointer width on MSVC.

#if defined(TOOL_ATOMIC_BUILTIN)
	inline void* AtomicPointerLoad(const AtomicPointer* atomic) { return AtomicRawLoad(&atomic->value); }
	inline void AtomicPointerStore(AtomicPointer* atomic, void* value) { AtomicRawStore(&atomic->value, value); }
	inline void* AtomicPointerExchange(AtomicPointer* atomic, void* value) { return AtomicRawExchange(&atomic->value, value); }
	inline b8 AtomicPointerCompareExchange(AtomicPointer* atomic, void** expected, void* desired) { return AtomicRawCompareExchange(&atomic->value, expected, desired); }
#else
#if defined(_WIN64)
	typedef u64 AtomicPointerBits;
#else
	typedef u32 AtomicPointerBits;
#endif
	inline void* AtomicPointerLoad(const AtomicPointer* atomic) { return (void*)AtomicRawLoad((const AtomicPointerBits*)&atomic->value); }
	inline void AtomicPointerStore(AtomicPointer* atomic, void* value) { AtomicRawStore((AtomicPointerBits*)&atomic->value, (AtomicPointerBits)value); }
	inline void* AtomicPointerExchange(AtomicPointer* atomic, void* value) { return (void*)AtomicRawExchange((AtomicPointerBits*)&atomic->value, (AtomicPointerBits)value); }
	inline b8 AtomicPointerCompareExchange(AtomicPointer* atomic, void** expected, void* desired) { return AtomicRawCompareExchange((AtomicPointerBits*)&atomic->value, (AtomicPointerBits*)expected, (AtomicPointerBits)desired); }
#endif
} //namespace Tool



#endif //_TOOL_ATOMIC_H
