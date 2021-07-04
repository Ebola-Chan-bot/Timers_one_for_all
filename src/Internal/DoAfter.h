#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{	
	//在指定毫秒数后执行任务
	template <uint8_t TimerCode, uint16_t AfterMilliseconds, void (*DoTask)()>
	void DoAfter()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, AfterMilliseconds);
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask, 1>();
	}
}