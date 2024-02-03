#pragma once
#include "Kernel.h"
namespace Timers_one_for_all
{
	#ifdef ARDUINO_ARCH_AVR
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, RepeatTimes, DoneCallback>();
	}
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t TimerCode, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(uint16_t IntervalMilliseconds, void (*DoTask)())
	{
		Internal::TimerSetting TS = Internal::GetTimerSetting<TimerCode>(IntervalMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, RepeatTimes, DoneCallback>(TS.TCNT, TS.PrescalerBits);
	}
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoneCallback)() = nullptr>
	void RepeatAfter(int32_t RepeatTimes, void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, DoneCallback>(RepeatTimes);
	}
	#endif
	#ifdef ARDUINO_ARCH_SAM
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, RepeatTimes, DoneCallback>();
	}
	#endif
}