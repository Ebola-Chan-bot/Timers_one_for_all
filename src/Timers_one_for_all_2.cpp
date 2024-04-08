#include "Timers_one_for_all_2.hpp"
using namespace Timers_one_for_all;
_TimerState Timers_one_for_all::_TimerStates[NumTimers];
const TimerClass *Timers_one_for_all::AllocateIdleTimer()
{
	for (int8_t T = NumTimers - 1; T >= 0; --T)
		if (!HardwareTimers[T]->Busy())
			return HardwareTimers[T];
	return nullptr;
}
#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0
ISR(TIMER0_COMPA_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].OVFCOMPA();
}
ISR(TIMER0_COMPB_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].COMPB();
}
#endif
#define TimerIsr(T)                                             \
	ISR(TIMER##T##_OVF_vect)                                    \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].OVFCOMPA(); \
	}                                                           \
	ISR(TIMER##T##_COMPA_vect)                                  \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].OVFCOMPA(); \
	}                                                           \
	ISR(TIMER##T##_COMPB_vect)                                  \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].COMPB();    \
	}
#ifdef TOFA_TIMER1
TimerIsr(1);
#endif
#ifdef TOFA_TIMER2
#warning 使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2
TimerIsr(2);
#endif
#ifdef TOFA_TIMER3
TimerIsr(3);
#endif
#ifdef TOFA_TIMER4
TimerIsr(4);
#endif
#ifdef TOFA_TIMER5
TimerIsr(5);
#endif
void TimerClass::Pause() const
{
	const uint8_t Clock = TCCRB & 0b111;
	if (Clock)
	{
		State.Clock = Clock;
		TCCRB &= ~0b111;
	}
}
void TimerClass::Continue() const
{
	if (!(TCCRB & 0b111))
		TCCRB |= State.Clock;
}
#endif