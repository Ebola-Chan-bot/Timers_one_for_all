#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	// 在指定毫秒数后执行任务
	template <uint8_t TimerCode, uint16_t AfterMilliseconds>
	void DoAfter(void (*DoTask)())
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, AfterMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, 1, nullptr>();
	}
	// 在指定毫秒数后执行任务
	template <uint8_t TimerCode>
	void DoAfter(uint16_t AfterMilliseconds, void (*DoTask)())
	{
		Internal::TimerSetting TS = Internal::GetTimerSetting<TimerCode>(AfterMilliseconds);
		Internal::TimerTask<TimerCode> = DoTask;
		Internal::SLRepeaterSet<TimerCode, 1, nullptr>(TS.TCNT, TS.PrescalerBits);
	}
}