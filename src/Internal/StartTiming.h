#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{	
	namespace Internal
	{		
		template <uint8_t TimerCode>
		void MEAdd()
		{
			MillisecondsElapsed<TimerCode> ++;
		}
	}
	//设置当前为零时刻进行计时。从MillisecondsElapsed变量读取经过的毫秒数
	template <uint8_t TimerCode>
	void StartTiming()
	{
		MillisecondsElapsed<TimerCode> = 0;
		RepeatAfter<TimerCode, 1, Internal::MEAdd<TimerCode>>();
	}
}