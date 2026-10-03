#ifndef _TOOL_TEMPORAL_H
#define _TOOL_TEMPORAL_H

#include "basics.h"



//~ Time
//
// Two clocks:
// - Timepoint is the monotonic clock, for measuring how long things take. It never jumps, but a single value means
//   nothing on its own: take the difference of two, through the functions here.
// - SystemTimepoint is the wall clock, in 100-nanosecond ticks since 1601 (Windows' FILETIME), taken as UTC or
//   local time. ClockTime splits one into a calendar date and a time of day.



namespace Tool
{
	//- Type definitions

	//~ Monotonic performance clock

	// Performance counter clocks on Windows, nanoseconds on Unix. Access through utility functions.
	typedef i64 Timepoint;

	struct Duration
	{
		i32 seconds;
		i32 nanoseconds;
	};

	//~ Wall/system clock
	// Local time is only precise to milliseconds on Windows.

	enum Weekday : u16
	{
		Monday = 0,
		Tuesday = 1,
		Wednesday = 2,
		Thursday = 3,
		Friday = 4,
		Saturday = 5,
		Sunday = 6
	};

	// 100-nanosecond ticks since 1601-01-01 (the Windows FILETIME epoch), in UTC or local time as it was taken.
	// Subtracting two gives the ticks between them.
	typedef u64 SystemTimepoint;

	struct ClockTime
	{
		u16 year;
		u16 month;
		Weekday weekday;
		u16 day;

		u16 hour;
		u16 minute;
		u16 second;
		u16 millisecond;
	};

	// A span in fixed units. Years and months are left out, since their length varies.
	struct ClockDuration
	{
		u32 days;
		u16 hours;
		u16 minutes;
		u16 seconds;
		u16 milliseconds;
	};



	//- Function declarations

	//~ Monotonic measurement

	Timepoint TimepointNow();

	i64 NanosecondsFromTo(Timepoint from, Timepoint to);
	i64 NanosecondsSince(Timepoint then);

	i64 ClocksFromTo(Timepoint from, Timepoint to);
	i64 ClocksSince(Timepoint then);

	template<typename T> T SecondsFromTo(Timepoint from, Timepoint to);
	template<typename T> T SecondsSince(Timepoint then);

	//~ System time measurement

	// Local time respects timezones and daylight savings
	// Keep this in mind when using system timepoints
	SystemTimepoint SystemTimepointNow(b8 local);

	ClockTime ClockTimeNow(b8 local);
	ClockTime ClockTimeFromSystemTimepoint(SystemTimepoint timepoint);

	// The span from 'from' to 'to', which must not come before it. Both should be UTC, or both local.
	ClockDuration ClockDurationFromTo(SystemTimepoint from, SystemTimepoint to);
} //namespace Tool



//- Implementation

namespace Tool
{
	template<typename T>
	T SecondsFromTo(Timepoint from, Timepoint to)
	{
		return NanosecondsFromTo(from, to) / ((T)1000000000);
	}

	template<typename T>
	T SecondsSince(Timepoint then)
	{
		return NanosecondsSince(then) / ((T)1000000000);
	}
} //namespace Tool



#endif //_TOOL_TEMPORAL_H
