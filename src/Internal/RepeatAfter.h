#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	//每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), int32_t RepeatTimes = -1>
	void RepeatAfter()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask, RepeatTimes>();
	}
}