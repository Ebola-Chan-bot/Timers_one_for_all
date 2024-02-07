#include "Timers_one_for_all.hpp"
#include <array>
#include <memory>
void DecayedTOI(Timers_one_for_all::Advanced::TimerState &State)
{
	State.Arguments.TimingState.OverflowCount++;
}
#ifdef ARDUINO_ARCH_AVR
#define TIMERISR(Index) \
	ISR(TIMER##Index##_OVF_vect) { Timers_one_for_all::Advanced::TimerStates[Index].InterruptServiceRoutine(Timers_one_for_all::Advanced::TimerStates[Index]); }
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
void TimingOverflowIsr(Timers_one_for_all::Advanced::TimerState &State)
{
	auto &TS = State.Arguments.TimingState;
	const Timers_one_for_all::Advanced::TimerInfo *Timer = TS.Timer;
	TS.OverflowCount++;
	const uint8_t NextPC = Timer->TCCRB + 1;
	if (NextPC >= Timer->NumPrescaleClocks)
		State.InterruptServiceRoutine = DecayedTOI;
	else if (TS.OverflowCount * Timer->PrescaleClocks[NextPC - 1].Divisor >= Timer->PrescaleClocks[NextPC].Divisor)
	{
		Timer->TCCRB = NextPC;
		TS.OverflowCount = 1;
	}
}
#endif
#ifdef ARDUINO_ARCH_SAM
#define TIMERISR(Index) \
	void TC##Index##_Handler() { Timers_one_for_all::Advanced::TimerStates[Index].InterruptServiceRoutine(Timers_one_for_all::Advanced::TimerStates[Index]); }
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
void TimingOverflowIsr(Timers_one_for_all::Advanced::TimerState &State)
{
	auto &TS = State.Arguments.TimingState;
	if (++TS.OverflowCount >= 4)
	{
		RwReg &TC_CMR = TS.Timer->Channel.TC_CMR;
		if (TC_CMR & 0b11 < 3)
		{
			TC_CMR++;
			TS.OverflowCount = 1;
		}
		else
			State.InterruptServiceRoutine = DecayedTOI;
	}
}
#endif
namespace Timers_one_for_all
{
	namespace Advanced
	{
#ifdef ARDUINO_ARCH_AVR
		uint16_t TimerInfo::ReadCounter() const
		{
			if (CounterBits == 8)
				return *(uint8_t *)TCNT;
			else
				return *(uint16_t *)TCNT;
		}
		void TimerInfo::ClearCounter() const
		{
			if (CounterBits == 8)
				*(uint8_t *)TCNT = 0;
			else
				*(uint16_t *)TCNT = 0;
		}
#endif
#ifdef ARDUINO_ARCH_SAM
		void TimerInitialize(uint8_t Timer)
		{
			auto &T = Timers[Timer];
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
		void StartTiming(uint8_t Timer)
		{
			const TimerInfo &T = Timers[Timer];
#ifdef ARDUINO_ARCH_AVR
			T.TCCRB = 0;
			T.CtcRegister = 0;
			T.ClearCounter();
			T.TIFR = UINT8_MAX;
			T.TIMSK = TimerInfo::TOIE;
			TimerStates[Timer] = {.InterruptServiceRoutine = TimingOverflowIsr, .Arguments.TimingState = {.Timer = &T, .OverflowCount = 0}};
#endif
#ifdef ARDUINO_ARCH_SAM
			NVIC_ClearPendingIRQ(T.irq);
			TimerStates[Timer] = {.InterruptServiceRoutine = TimingOverflowIsr, .Arguments.TimingState = {.Timer = &T, .OverflowCount = 0}};
			T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK1, .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
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
		Advanced::StartTiming(Timer);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
}