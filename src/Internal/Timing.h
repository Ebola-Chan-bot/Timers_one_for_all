#pragma once
#include "RepeatAfter.h"
namespace Timers_one_for_all
{
	// 设为false可以暂停计时，重新设为true可继续计时
	template <uint8_t TimerCode>
	volatile bool Running;
	// 取得上次调用StartTiming以来经过的毫秒数
	template <uint8_t TimerCode>
	volatile uint32_t MillisecondsElapsed;
	namespace Internal
	{
		template <uint8_t TimerCode, uint16_t MillsecondsPerTick>
		void MEAdd()
		{
			if (Running<TimerCode>)
				MillisecondsElapsed<TimerCode> += MillsecondsPerTick;
		}
	}
	// 设置当前为零时刻进行计时。从MillisecondsElapsed变量读取经过的毫秒数
	template <uint8_t TimerCode, uint16_t MillisecondsPerTick = 1>
	void StartTiming(uint32_t TimeShift = 0)
	{
		RepeatAfter<TimerCode, MillisecondsPerTick>(Internal::MEAdd<TimerCode, MillisecondsPerTick>);
		MillisecondsElapsed<TimerCode> = TimeShift;
		Running<TimerCode> = true;
	}
}
