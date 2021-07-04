#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{	
	//取消上述任何任务。只影响特定的计时器。
	template <uint8_t TimerCode>
	void ShutDown()
	{
		Internal::TIMSK<TimerCode> = 0;
	}
}