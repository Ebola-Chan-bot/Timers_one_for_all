#include "Timers_one_for_all.hpp"
#include <Arduino.h>
#include <map>
using namespace Timers_one_for_all;
#ifdef ARDUINO_ARCH_AVR
struct TimerBase
{
	volatile uint8_t &TIMSK;
};
struct TimerType0 : public TimerBase
{
};
struct TimerType1 : public TimerBase
{
};
struct TimerType2 : public TimerBase
{
};
#ifdef TOFA_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0
constexpr TimerType0 AvrTimer0{TIMSK0};
#endif
#ifdef TOFA_TIMER1
constexpr TimerType1 AvrTimer1{TIMSK1};
#endif
#ifdef TOFA_TIMER2
#warning 使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2
constexpr TimerType2 AvrTimer2{TIMSK2};
#endif
#ifdef TOFA_TIMER3
constexpr TimerType1 AvrTimer3{TIMSK3};
#endif
#ifdef TOFA_TIMER4
constexpr TimerType1 AvrTimer4{TIMSK4};
#endif
#ifdef TOFA_TIMER5
constexpr TimerType1 AvrTimer5{TIMSK5};
#endif
constexpr const TimerBase *AvrTimers[(size_t)HardwareTimer::NumTimers] = {
#ifdef TOFA_TIMER0
	&AvrTimer0,
#endif
#ifdef TOFA_TIMER1
	&AvrTimer1,
#endif
#ifdef TOFA_TIMER2
	&AvrTimer2,
#endif
#ifdef TOFA_TIMER3
	&AvrTimer3,
#endif
#ifdef TOFA_TIMER4
	&AvrTimer4,
#endif
#ifdef TOFA_TIMER5
	&AvrTimer5,
#endif
};
Exception Timers_one_for_all::IsTimerBusy(HardwareTimer Timer, bool &TimerBusy)
{
	if (Timer < HardwareTimer::NumTimers)
	{
		TimerBusy = AvrTimers[(size_t)Timer]->TIMSK;
		return Exception::Successful_operation;
	}
	else
		return Exception::Invalid_timer;
}
Exception Timers_one_for_all::Pause(HardwareTimer Timer)
{
	switch(Timer)
	{
		
	}
}
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_USE_SYSTIMER
#warning 使用系统计时器可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_SYSTIMER
#endif
#endif