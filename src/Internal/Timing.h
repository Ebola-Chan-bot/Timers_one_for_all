#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	//设为false可以暂停计时，重新设为true可继续计时
	template <uint8_t TimerCode>
	static volatile bool Running;
	namespace Internal
	{
		template <uint8_t TimerCode>
		void MEAdd()
		{
			if (Running<TimerCode>)
				MillisecondsElapsed<TimerCode> ++;
		}
	}
	//设置当前为零时刻进行计时。从MillisecondsElapsed变量读取经过的毫秒数
	template <uint8_t TimerCode>
	void StartTiming()
	{
		RepeatAfter<TimerCode, 1, Internal::MEAdd<TimerCode>>();
		MillisecondsElapsed<TimerCode> = 0;
		Running<TimerCode> = true;
	}
}