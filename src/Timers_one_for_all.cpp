#include "Timers_one_for_all.hpp"
#include <array>
#include <memory>
struct TimerState
{
	void (*InterruptServiceRoutine)(TimerState &State);
	const Timers_one_for_all::Advanced::TimerInfo *Timer;
	size_t OverflowCount;
} TimerStates[Timers_one_for_all::Advanced::NumTimers];
#ifdef ARDUINO_ARCH_AVR
#define TIMERISR(Index) \
	ISR(TIMER##Index##_OVF_vect) { TimerStates[Index].InterruptServiceRoutine(TimerStates[Index]); }
#ifndef TOFA_DISABLE_0
TIMERISR(0);
#endif
#ifndef TOFA_DISABLE_1
TIMERISR(1);
#endif
#ifndef TOFA_DISABLE_2
TIMERISR(2);
#endif
#ifdef __AVR_ATmega2560__
#ifndef TOFA_DISABLE_3
TIMERISR(3);
#endif
#ifndef TOFA_DISABLE_4
TIMERISR(4);
#endif
#ifndef TOFA_DISABLE_5
TIMERISR(5);
#endif
#endif
void TimingOverflowIsr(TimerState &State)
{
	const Timers_one_for_all::Advanced::TimerInfo *Timer = State.Timer;
	const uint8_t PD = Timer->TCCRB;
	if (PD + 1 < Timer->NumPrescaleDivisors)
		Timer->SetCounter((1 << Timer->CounterBytes * 8) * Timer->PrescaleDivisors[PD] / Timer->PrescaleDivisors[Timer->TCCRB = PD + 1]);
	else
		State.OverflowCount++;
}
#endif
#ifdef ARDUINO_ARCH_SAM
#define TIMERISR(Index) \
	void TC##Index##_Handler() { TimerStates[Index].InterruptServiceRoutine(TimerStates[Index]); }
#ifndef TOFA_DISABLE_0
TIMERISR(0);
#endif
#ifndef TOFA_DISABLE_1
TIMERISR(1);
#endif
#ifndef TOFA_DISABLE_2
TIMERISR(2);
#endif
#ifndef TOFA_DISABLE_3
TIMERISR(3);
#endif
#ifndef TOFA_DISABLE_4
TIMERISR(4);
#endif
#ifndef TOFA_DISABLE_5
TIMERISR(5);
#endif
#ifndef TOFA_DISABLE_6
TIMERISR(6);
#endif
#ifndef TOFA_DISABLE_7
TIMERISR(7);
#endif
#ifndef TOFA_DISABLE_8
TIMERISR(8);
#endif
void TimingOverflowIsr(TimerState &State)
{
	const Timers_one_for_all::Advanced::TimerInfo *Timer = State.Timer;
	const uint8_t PD = Timer->TCCRB;
	if (PD + 1 < Timer->NumPrescaleDivisors)
		Timer->SetCounter((1 << Timer->CounterBytes * 8) * Timer->PrescaleDivisors[PD] / Timer->PrescaleDivisors[Timer->TCCRB = PD + 1]);
	else
		State.OverflowCount++;
}
#endif
namespace Timers_one_for_all
{
	namespace Advanced
	{
#ifdef ARDUINO_ARCH_AVR
		uint16_t TimerInfo::ReadCounter() const
		{
			if (CounterBytes == 1)
				return *(uint8_t *)TCNT;
			else
				return *(uint16_t *)TCNT;
		}
		void TimerInfo::SetCounter(uint16_t Counter) const
		{
			if (CounterBytes == 1)
				*(uint8_t *)TCNT = Counter;
			else
				*(uint16_t *)TCNT = Counter;
		}
#endif
#ifdef ARDUINO_ARCH_SAM
		void TimerInitialize(uint8_t Timer)
		{
			auto &T = Timers[Timer];
			T.tc->TC_CHANNEL[T.channel].TC_IER = TC_IER_CPCS;
			T.tc->TC_CHANNEL[T.channel].TC_IDR = ~TC_IER_CPCS;
			NVIC_ClearPendingIRQ(T.irq);
			NVIC_EnableIRQ(T.irq);
		}
#endif
		Exception AllocateTimer(uint8_t &Timer)
		{
			Timer = NumTimers - 1;
			do
			{
				if (!TimerBusy[Timer])
				{
					TimerBusy[Timer] = true;
					return Exception::Successful_operation;
				}
			} while (--Timer < NumTimers);
			return Exception::No_idle_timer;
		}
		Exception StartTiming(uint8_t Timer)
		{
			const TimerInfo &T = Timers[Timer];
#ifdef ARDUINO_ARCH_AVR
			T.TIMSK = 0;
			T.TCCRA = 0;
			T.TCCRB = 0;
			TimerStates[Timer] = {.InterruptServiceRoutine = TimingOverflowIsr, .Timer = &T, .OverflowCount = 0};
			T.SetCounter(0);
			T.TIFR = 255;
			T.TIMSK = 1;
#endif
#ifdef ARDUINO_ARCH_SAM

#endif
		}
	}
	void FreeTimer(uint8_t Timer)
	{
		if (Timer < Advanced::NumTimers)
		{
			Advanced::Shutdown(Timer);
			Advanced::TimerBusy[Timer] = false;
		}
	}
	Exception StartTiming(uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::StartTiming(Timer);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
}