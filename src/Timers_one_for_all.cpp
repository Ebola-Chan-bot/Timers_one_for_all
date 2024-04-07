#include "Timers_one_for_all.hpp"
#include <Arduino.h>
#include <map>
using namespace Timers_one_for_all;
#ifdef ARDUINO_ARCH_AVR
struct TimerBase
{
	volatile uint8_t &TIMSK;
	volatile uint8_t &TCCRA;
	volatile uint8_t &TCCRB;
	volatile uint8_t &TIFR;
	virtual void ClearTCNT() const = 0;
	virtual void UseOverflow() const
	{
		TIMSK = 1 << TOIE0;
	}
	constexpr TimerBase(volatile uint8_t &TIMSK, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIFR) : TIMSK(TIMSK), TCCRA(TCCRA), TCCRB(TCCRB), TIFR(TIFR) {}
};
template <typename T>
struct TypedCounter : public TimerBase
{
	volatile T &TCNT;
	volatile T &OCRA;
	volatile T &OCRB;
	void ClearTCNT() const override
	{
		TCNT = 0;
	}
	constexpr TypedCounter(volatile uint8_t &TIMSK, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIFR, volatile T &TCNT, volatile T &OCRA, volatile T &OCRB) : TimerBase(TIMSK, TCCRA, TCCRB, TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB) {}
};
struct CommonPrescalers
{
	// 使用左移运算应用预分频
	static constexpr uint8_t PrescalerShift[] = {3, 3, 2, 2};
	static constexpr uint8_t NumPrescalers = std::extent_v<decltype(PrescalerShift)>;
};
struct TimerType0 : public TypedCounter<uint8_t>, public CommonPrescalers
{
	void UseOverflow() const override
	{
		OCRA = 0;
		TIMSK = 1 << OCIE0A;
	}
	constexpr TimerType0(volatile uint8_t &TIMSK, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIFR, volatile uint8_t &TCNT, volatile uint8_t &OCRA, volatile uint8_t &OCRB) : TypedCounter<uint8_t>(TIMSK, TCCRA, TCCRB, TIFR, TCNT, OCRA, OCRB) {}
};
struct TimerType1 : public TypedCounter<uint16_t>, public CommonPrescalers
{
	constexpr TimerType1(volatile uint8_t &TIMSK, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIFR, volatile uint16_t &TCNT, volatile uint16_t &OCRA, volatile uint16_t &OCRB) : TypedCounter<uint16_t>(TIMSK, TCCRA, TCCRB, TIFR, TCNT, OCRA, OCRB) {}
};
struct TimerType2 : public TypedCounter<uint8_t>
{
	static constexpr uint8_t PrescalerShift[] = {3, 2, 1, 1, 1, 2};
	static constexpr uint8_t NumPrescalers = std::extent_v<decltype(PrescalerShift)>;
	constexpr TimerType2(volatile uint8_t &TIMSK, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIFR, volatile uint8_t &TCNT, volatile uint8_t &OCRA, volatile uint8_t &OCRB) : TypedCounter<uint8_t>(TIMSK, TCCRA, TCCRB, TIFR, TCNT, OCRA, OCRB) {}
};
static struct
{
	uint8_t Clock;
	std::function<void()> OVFCOMPA;
	std::function<void()> COMPB;
	unsigned long OverflowCount;
} TimerStates[(size_t)HardwareTimer::NumTimers];
#ifdef TOFA_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0
constexpr TimerType0 AvrTimer0{TIMSK0, TCCR0A, TCCR0B, TIFR0, TCNT0, OCR0A, OCR0B};
ISR(TIMER0_COMPA_vect)
{
	TimerStates[(size_t)HardwareTimer::Timer0].OVFCOMPA();
}
ISR(TIMER0_COMPB_vect)
{
	TimerStates[(size_t)HardwareTimer::Timer0].COMPB();
}
#endif
#define TimerIsr(T)                                                \
	ISR(TIMER##T##_OVF_vect)                                       \
	{                                                              \
		TimerStates[(size_t)HardwareTimer::Timer##T##].OVFCOMPA(); \
	}                                                              \
	ISR(TIMER##T##_COMPA_vect)                                     \
	{                                                              \
		TimerStates[(size_t)HardwareTimer::Timer##T##].OVFCOMPA(); \
	}                                                              \
	ISR(TIMER##T##_COMPB_vect)                                     \
	{                                                              \
		TimerStates[(size_t)HardwareTimer::Timer##T##].COMPB();    \
	}
#ifdef TOFA_TIMER1
constexpr TimerType1 AvrTimer1{TIMSK1, TCCR1A, TCCR1B, TIFR1, TCNT1, OCR1A, OCR1B};
TimerIsr(1);
#endif
#ifdef TOFA_TIMER2
#warning 使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2
constexpr TimerType2 AvrTimer2{TIMSK2, TCCR2A, TCCR2B, TIFR2, TCNT2, OCR2A, OCR2B};
TimerIsr(2);
#endif
#ifdef TOFA_TIMER3
constexpr TimerType1 AvrTimer3{TIMSK3, TCCR3A, TCCR3B, TIFR3, TCNT3, OCR3A, OCR3B};
TimerIsr(3);
#endif
#ifdef TOFA_TIMER4
constexpr TimerType1 AvrTimer4{TIMSK4, TCCR4A, TCCR4B, TIFR4, TCNT4, OCR4A, OCR4B};
TimerIsr(4);
#endif
#ifdef TOFA_TIMER5
constexpr TimerType1 AvrTimer5{TIMSK5, TCCR5A, TCCR5B, TIFR5, TCNT5, OCR5A, OCR5B};
TimerIsr(5);
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
#define TOFA_CHECKTIMER                    \
	if (Timer >= HardwareTimer::NumTimers) \
		return Exception::Invalid_timer;
Exception Timers_one_for_all::IsTimerBusy(HardwareTimer Timer, bool &TimerBusy)
{
	TOFA_CHECKTIMER;
	TimerBusy = AvrTimers[(size_t)Timer]->TIMSK;
	return Exception::Successful_operation;
}
Exception Timers_one_for_all::Pause(HardwareTimer Timer)
{
	TOFA_CHECKTIMER;
	uint8_t &TCCRB = (uint8_t &)AvrTimers[(size_t)Timer]->TCCRB;
	const uint8_t Clock = TCCRB & 0b111;
	if (Clock)
	{
		TimerStates[(size_t)Timer].Clock = TCCRB & 0b111;
		TCCRB &= ~0b111;
		return Exception::Successful_operation;
	}
	else
		return Exception::Timer_already_paused;
}
Exception Timers_one_for_all::Continue(HardwareTimer Timer)
{
	TOFA_CHECKTIMER;
	uint8_t &TCCRB = (uint8_t &)AvrTimers[(size_t)Timer]->TCCRB;
	if (TCCRB & 0b111)
		return Exception::Timer_still_running;
	TCCRB |= TimerStates[(size_t)Timer].Clock;
	return Exception::Successful_operation;
}
Exception Timers_one_for_all::Stop(HardwareTimer Timer)
{
	TOFA_CHECKTIMER;
	AvrTimers[(size_t)Timer]->TIMSK = 0;
	return Exception::Successful_operation;
}
Exception AllocateTimer(HardwareTimer *Timer)
{
	if (!Timer)
		return Exception::Null_pointer_specified;
	size_t &T = *(size_t *)Timer;
	for (T = 0; *Timer < HardwareTimer::NumTimers; ++T)
		if (!AvrTimers[T]->TIMSK)
			return Exception::Successful_operation;
	return Exception::All_timers_busy;
}
Exception Timers_one_for_all::StartTiming(HardwareTimer Timer)
{
	TOFA_CHECKTIMER;
	const TimerBase *T = AvrTimers[(size_t)Timer];
	T->ClearTCNT();
	T->TCCRB = 1;
	T->TCCRA = 0;
	T->UseOverflow();
	TimerStates[(size_t)Timer].OverflowCount = 0;
#ifdef TOFA_TIMER2
	if (Timer == HardwareTimer::Timer2)
		TimerStates[(size_t)Timer].OVFCOMPA = []()
		{
			auto &TS = TimerStates[(size_t)HardwareTimer::Timer2];
			unsigned long &OverflowCount = TS.OverflowCount;
			uint8_t &TCCRB = (uint8_t &)AvrTimer2.TCCRB;
			if (++OverflowCount >> TimerType2::PrescalerShift[TCCRB])
			{
				TCCRB++;
				OverflowCount = 1;
				if (TCCRB == TimerType2::NumPrescalers)
					TS.OVFCOMPA = [&OverflowCount]()
					{ OverflowCount++; };
			}
		};
	else
#endif
		TimerStates[(size_t)Timer].OVFCOMPA = [Timer]()
		{
			auto &TS = TimerStates[(size_t)Timer];
			unsigned long &OverflowCount = TS.OverflowCount;
			uint8_t &TCCRB = (uint8_t &)AvrTimers[(size_t)Timer]->TCCRB;
			if (++OverflowCount >> CommonPrescalers::PrescalerShift[TCCRB])
			{
				TCCRB++;
				OverflowCount = 1;
				if (TCCRB == CommonPrescalers::NumPrescalers)
					TS.OVFCOMPA = [&OverflowCount]()
					{ OverflowCount++; };
			}
		};
	T->TIFR = -1;
	return Exception::Successful_operation;
}
#define TOFA_ALLOCATE_AND_FORWARD(Forward)    \
	const Exception E = AllocateTimer(Timer); \
	return E == Exception::Successful_operation ? Forward : E;
Exception Timers_one_for_all::StartTiming(HardwareTimer *Timer)
{
	TOFA_ALLOCATE_AND_FORWARD(StartTiming(*Timer));
}
Exception Timers_one_for_all::GetTiming(HardwareTimer Timer, Tick &Timing)
{
	TOFA_CHECKTIMER;
	TimerStates[(size_t)Timer].OverflowCount
	AvrTimers[(size_t)Timer]->GetTCNT()
}
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_USE_SYSTIMER
#warning 使用系统计时器可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_SYSTIMER
#endif
#endif