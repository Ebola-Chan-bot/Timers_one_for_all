#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	namespace Internal
	{
		template <uint8_t TimerCode>
		volatile uint8_t PauseTIMSK;
		template <uint8_t TimerCode>
		volatile uint16_t PauseTCNT;
	}
	// 取消上述任何任务。只影响特定的计时器。
	template <uint8_t TimerCode>
	void ShutDown()
	{
		Internal::TIMSK<TimerCode> = 0;
	}
	//暂停指定计时器上的任务
	template <uint8_t TimerCode>
	void Pause()
	{
		Internal::PauseTIMSK<TimerCode> = Internal::TIMSK<TimerCode>;
		Internal::TIMSK<TimerCode> = 0;
		Internal::PauseTCNT<TimerCode> = Internal::GetTCNT<TimerCode>();
	}
	//继续指定计时器上的任务。如果继续一个未处于暂停状态的计时器，将产生未知行为。
	template <uint8_t TimerCode>
	void Continue()
	{
		Internal::SetTCNT<TimerCode>(Internal::PauseTCNT<TimerCode>);
		Internal::TIMSK<TimerCode> = Internal::PauseTIMSK<TimerCode>;
	}
}