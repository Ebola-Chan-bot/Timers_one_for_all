#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	namespace Internal
	{
		template <uint8_t TimerCode>
		static volatile bool Delaying;
		template <uint8_t TimerCode>
		void Undelay()
		{
			Delaying<TimerCode> = false;
		}
	}
	//阻塞当前代码执行指定毫秒数
	template <uint8_t TimerCode, uint16_t DelayMilliseconds>
	void Delay()
	{
		Internal::Delaying<TimerCode> = true;
		DoAfter<TimerCode, DelayMilliseconds, Internal::Undelay<TimerCode>>();
		while (Internal::Delaying<TimerCode>)
			;
	}
}