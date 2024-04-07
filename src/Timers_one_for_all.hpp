#pragma once
#include <chrono>
#include <functional>
#define TOFA_TIMER0
#define TOFA_TIMER1
#define TOFA_TIMER2
#define TOFA_TIMER3
#define TOFA_TIMER4
#define TOFA_TIMER5
#ifdef ARDUINO_ARCH_SAM
#define TOFA_TIMER6
#define TOFA_TIMER7
#define TOFA_TIMER8
#define TOFA_SYSTIMER
#endif
namespace Timers_one_for_all
{
	enum class HardwareTimer
	{
#ifdef TOFA_TIMER0
		Timer0,
#endif
#ifdef TOFA_TIMER1
		Timer1,
#endif
#ifdef TOFA_TIMER2
		Timer2,
#endif
#ifdef TOFA_TIMER3
		Timer3,
#endif
#ifdef TOFA_TIMER4
		Timer4,
#endif
#ifdef TOFA_TIMER5
		Timer5,
#endif
#ifdef TOFA_TIMER6
		Timer6,
#endif
#ifdef TOFA_TIMER7
		Timer7,
#endif
#ifdef TOFA_TIMER8
		Timer8,
#endif
#ifdef TOFA_SYSTIMER
		SysTimer,
#endif
		NumTimers,
	};
	enum class Exception
	{
		Successful_operation,
		Invalid_timer,
	};
	// 查询计时器是否忙碌
	Exception IsTimerBusy(HardwareTimer, bool &TimerBusy);
	// 暂停计时器，而不将其设为空闲
	Exception Pause(HardwareTimer);
	// 继续已暂停的计时器
	Exception Continue(HardwareTimer);
	// 停止计时器并将其设为空闲
	Exception Stop(HardwareTimer);
	// 使用指定Timer开始计时
	Exception StartTiming(HardwareTimer);
	// 分配空闲Timer开始计时
	Exception StartTiming(HardwareTimer *);
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	Exception GetTiming(HardwareTimer, Tick &);
	// 获取指定Timer已经记录的时长
	template <typename _Rep, typename _Period>
	inline Exception GetTiming(HardwareTimer Timer, std::chrono::duration<_Rep, _Period> &Time)
	{
		Tick TickElapsed;
		const Exception E = GetTiming(Timer, TickElapsed);
		Time = std::chrono::duration_cast<decltype(Time)>(TickElapsed);
		return E;
	}
	Exception Delay(Tick, HardwareTimer);
	Exception Delay(Tick);
	// 使用指定Timer阻塞当前线程
	template <typename _Rep, typename _Period>
	inline Exception Delay(std::chrono::duration<_Rep, _Period> Time, HardwareTimer Timer)
	{
		return Delay(std::chrono::duration_cast<Tick>(Time), Timer);
	}
	// 分配空闲Timer阻塞当前线程
	template <typename _Rep, typename _Period>
	inline Exception Delay(std::chrono::duration<_Rep, _Period> Time)
	{
		return Delay(std::chrono::duration_cast<Tick>(Time));
	}
	Exception DoAfter(HardwareTimer, Tick, std::function<void()>);
	Exception DoAfter(HardwareTimer *, Tick, std::function<void()>);
	// 使用指定Timer，在经过After时间后执行动作Do
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(HardwareTimer Timer, std::chrono::duration<_Rep, _Period> After, std::function<void()> Do)
	{
		return DoAfter(Timer, std::chrono::duration_cast<Tick>(After), Do);
	}
	// 分配空闲Timer，在经过After时间后执行动作Do
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(HardwareTimer *Timer, std::chrono::duration<_Rep, _Period> After, std::function<void()> Do)
	{
		return DoAfter(Timer, std::chrono::duration_cast<Tick>(After), Do);
	}
	// 使用此值作为重复次数参数，表示无限重复。无限重复只能用Stop结束，并且不会执行DoneCallback。
	constexpr size_t InfiniteRepeat = std::numeric_limits<size_t>::max();
	Exception RepeatEvery(HardwareTimer, Tick Every, std::function<void()> Repeat, size_t RepeatTimes, std::function<void()> DoneCallback);
	Exception RepeatEvery(HardwareTimer *, Tick Every, std::function<void()> Repeat, size_t RepeatTimes, std::function<void()> DoneCallback);
	Exception RepeatEvery(HardwareTimer, Tick Every, std::function<void()> Repeat, Tick RepeatDuration, std::function<void()> DoneCallback);
	Exception RepeatEvery(HardwareTimer *, Tick Every, std::function<void()> Repeat, Tick RepeatDuration, std::function<void()> DoneCallback);
	// 使用指定Timer，每隔Every周期执行Repeat动作，并指定重复次数RepeatTimes。第一次Repeat发生在Every时间之后。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception RepeatEvery(
		HardwareTimer Timer, std::chrono::duration<_Rep, _Period> Every, std::function<void()> Repeat, size_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {})
	{
		return RepeatEvery(Timer, Every, Repeat, RepeatTimes, DoneCallback);
	}
	// 分配空闲Timer，每隔Every周期执行Repeat动作，并指定重复次数RepeatTimes。第一次Repeat发生在Every时间之后。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception RepeatEvery(
		HardwareTimer *Timer, std::chrono::duration<_Rep, _Period> Every, std::function<void()> Repeat, size_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {})
	{
		return RepeatEvery(Timer, Every, Repeat, RepeatTimes, DoneCallback);
	}
	// 使用指定Timer，每隔Every周期执行Repeat动作，并指定持续时长RepeatDuration。第一次Repeat发生在Every时间之后。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception RepeatEvery(
		HardwareTimer Timer, std::chrono::duration<_Rep, _Period> Every, std::function<void()> Repeat, std::chrono::duration<_Rep, _Period> RepeatDuration, std::function<void()> DoneCallback = []() {})
	{
		return RepeatEvery(Timer, Every, Repeat, std::chrono::duration_cast<Tick>(RepeatDuration), DoneCallback);
	}
	// 分配空闲Timer，每隔Every周期执行Repeat动作，并指定持续时长RepeatDuration。第一次Repeat发生在Every时间之后。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception RepeatEvery(
		HardwareTimer *Timer, std::chrono::duration<_Rep, _Period> Every, std::function<void()> Repeat, std::chrono::duration<_Rep, _Period> RepeatDuration, std::function<void()> DoneCallback = []() {})
	{
		return RepeatEvery(Timer, Every, Repeat, std::chrono::duration_cast<Tick>(RepeatDuration), DoneCallback);
	}
	Exception DoubleRepeat(HardwareTimer, Tick AfterA, std::function<void()> RepeatA, Tick AfterB, std::function<void()> RepeatB, size_t NumHalfPeriods, std::function<void()> DoneCallback);
	Exception DoubleRepeat(HardwareTimer *, Tick AfterA, std::function<void()> RepeatA, Tick AfterB, std::function<void()> RepeatB, size_t NumHalfPeriods, std::function<void()> DoneCallback);
	Exception DoubleRepeat(HardwareTimer, Tick AfterA, std::function<void()> RepeatA, Tick AfterB, std::function<void()> RepeatB, Tick RepeatDuration, std::function<void()> DoneCallback);
	Exception DoubleRepeat(HardwareTimer *, Tick AfterA, std::function<void()> RepeatA, Tick AfterB, std::function<void()> RepeatB, Tick RepeatDuration, std::function<void()> DoneCallback);
	// 使用指定Timer，在AfterA后执行RepeatA，再在AfterB后执行RepeatB，再回到AfterA后再执行RepeatA……如此交替循环重复。NumHalfPeriods表示RepeatA和RepeatB执行的次数之和；如NumHalfPeriods为奇数，最后一次RepeatA之后将不再RepeatB，因而将比RepeatB多执行一次。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception DoubleRepeat(
		HardwareTimer Timer, std::chrono::duration<_Rep, _Period> AfterA, std::function<void()> RepeatA, std::chrono::duration<_Rep, _Period> AfterB, std::function<void()> RepeatB, size_t NumHalfPeriods = InfiniteRepeat, std::function<void()> DoneCallback = []() {})
	{
		return DoubleRepeat(Timer, std::chrono::duration_cast<Tick>(AfterA), RepeatA, std::chrono::duration_cast<Tick>(AfterB), RepeatB, NumHalfPeriods, DoneCallback);
	}
	// 分配空闲Timer，在AfterA后执行RepeatA，再在AfterB后执行RepeatB，再回到AfterA后再执行RepeatA……如此交替循环重复。NumHalfPeriods表示RepeatA和RepeatB执行的次数之和；如NumHalfPeriods为奇数，最后一次RepeatA之后将不再RepeatB，因而将比RepeatB多执行一次。所有重复结束后执行DoneCallback
	template <typename _Rep, typename _Period>
	inline Exception DoubleRepeat(
		HardwareTimer *Timer, std::chrono::duration<_Rep, _Period> AfterA, std::function<void()> RepeatA, std::chrono::duration<_Rep, _Period> AfterB, std::function<void()> RepeatB, size_t NumHalfPeriods = InfiniteRepeat, std::function<void()> DoneCallback = []() {})
	{
		return DoubleRepeat(Timer, std::chrono::duration_cast<Tick>(AfterA), RepeatA, std::chrono::duration_cast<Tick>(AfterB), RepeatB, NumHalfPeriods, DoneCallback);
	}
	// 使用指定Timer，在AfterA后执行RepeatA，再在AfterB后执行RepeatB，再回到AfterA后再执行RepeatA……如此交替循环重复。RepeatDuration指定整个重复过程的总时长，到时间后立即结束并执行DoneCallback，不保证RepeatA和RepeatB都被执行了相同次数。
	template <typename _Rep, typename _Period>
	inline Exception DoubleRepeat(
		HardwareTimer Timer, std::chrono::duration<_Rep, _Period> AfterA, std::function<void()> RepeatA, std::chrono::duration<_Rep, _Period> AfterB, std::function<void()> RepeatB, std::chrono::duration<_Rep, _Period> RepeatDuration, std::function<void()> DoneCallback = []() {})
	{
		return DoubleRepeat(Timer, std::chrono::duration_cast<Tick>(AfterA), RepeatA, std::chrono::duration_cast<Tick>(AfterB), RepeatB, std::chrono::duration_cast<Tick>(RepeatDuration), DoneCallback);
	}
	// 分配空闲Timer，在AfterA后执行RepeatA，再在AfterB后执行RepeatB，再回到AfterA后再执行RepeatA……如此交替循环重复。RepeatDuration指定整个重复过程的总时长，到时间后立即结束并执行DoneCallback，不保证RepeatA和RepeatB都被执行了相同次数。
	template <typename _Rep, typename _Period>
	inline Exception DoubleRepeat(
		HardwareTimer *Timer, std::chrono::duration<_Rep, _Period> AfterA, std::function<void()> RepeatA, std::chrono::duration<_Rep, _Period> AfterB, std::function<void()> RepeatB, std::chrono::duration<_Rep, _Period> RepeatDuration, std::function<void()> DoneCallback = []() {})
	{
		return DoubleRepeat(Timer, std::chrono::duration_cast<Tick>(AfterA), RepeatA, std::chrono::duration_cast<Tick>(AfterB), RepeatB, std::chrono::duration_cast<Tick>(RepeatDuration), DoneCallback);
	}
}