#pragma once
#include "Kernel.h"
namespace Timers_one_for_all
{
#ifdef ARDUINO_ARCH_AVR
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t Timer, uint16_t IntervalMilliseconds, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(Timer, IntervalMilliseconds);
		Internal::TimerTask<Timer> = DoTask;
		Internal::SLRepeaterSet<Timer, TS.TCNT, TS.PrescalerBits, RepeatTimes, DoneCallback>();
	}
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t Timer, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(uint16_t IntervalMilliseconds, void (*DoTask)())
	{
		Internal::TimerSetting TS = Internal::GetTimerSetting<Timer>(IntervalMilliseconds);
		Internal::TimerTask<Timer> = DoTask;
		Internal::SLRepeaterSet<Timer, RepeatTimes, DoneCallback>(TS.TCNT, TS.PrescalerBits);
	}
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t Timer, uint16_t IntervalMilliseconds, void (*DoneCallback)() = nullptr>
	void RepeatAfter(int32_t RepeatTimes, void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(Timer, IntervalMilliseconds);
		Internal::TimerTask<Timer> = DoTask;
		Internal::SLRepeaterSet<Timer, TS.TCNT, TS.PrescalerBits, DoneCallback>(RepeatTimes);
	}
#endif
#ifdef ARDUINO_ARCH_SAM
	// 每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t Timer, uint16_t IntervalMilliseconds, int32_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void RepeatAfter(void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting_s<IntervalMilliseconds>::TS;
		pmc_set_writeprotect(false);
		Internal::TimerTask<Timer> = DoTask;
		Internal::SLRepeaterSet<Timer, TS.TCNT, TS.PrescalerBits, RepeatTimes, DoneCallback>();
	}
#endif
}