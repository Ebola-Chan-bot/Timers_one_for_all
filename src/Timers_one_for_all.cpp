#include "Timers_one_for_all.hpp"
#include <array>
#include <memory>
void DecayedTOI(Timers_one_for_all::Advanced::TimerState &State)
{
	State.OverflowCount++;
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
	const Timers_one_for_all::Advanced::TimerInfo &Timer = State.Timer;
	State.OverflowCount++;
	const uint8_t NextPC = Timer.TCCRB + 1;
	if (NextPC >= Timer.NumPrescaleClocks)
		State.InterruptServiceRoutine = DecayedTOI;
	else if (State.OverflowCount * Timer.PrescaleClocks[NextPC - 1].Divisor >= Timer.PrescaleClocks[NextPC].Divisor)
	{
		Timer.TCCRB = NextPC;
		State.OverflowCount = 1;
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
	if (++State.OverflowCount >= 4)
	{
		RwReg &TC_CMR = State.Timer.Channel.TC_CMR;
		if (TC_CMR & 0b11 < 3)
		{
			TC_CMR++;
			State.OverflowCount = 1;
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
		template <typename T>
		struct MakeTimerState;
		template <size_t... T>
		struct MakeTimerState<std::index_sequence<T...>>
		{
			static TimerState value[sizeof...(T)] = {{.Timer = Timers[T]}...};
		};
		TimerState (&TimerStates)[NumTimers] = MakeTimerState<std::make_index_sequence<NumTimers>>::value;
		Exception AllocateTimer(uint8_t &Timer)
		{
			Timer = NumTimers - 1;
			do
			{
				if (TimerStates[Timer].Free && TimerStates[Timer].AutoAlloFree)
				{
					TimerStates[Timer].Free = false;
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
			// 由于TCCRB和CtcRegister可能是同一个寄存器，上述两行的顺序不能改变

			T.ClearCounter();
			T.TIFR = UINT8_MAX;
			T.TIMSK = TimerInfo::TOIE;
#endif
#ifdef ARDUINO_ARCH_SAM
			NVIC_ClearPendingIRQ(T.irq);
			T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK1, .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
#endif
			TimerStates[Timer].OverflowCount = 0;
			TimerStates[Timer].InterruptServiceRoutine = TimingOverflowIsr;
		}
		void DoAfterIsr(TimerState &TS)
		{
#ifdef ARDUINO_ARCH_AVR
			// AVR架构必须手动终止计时
			TS.Timer.TIMSK = 0;
#endif
			// SAM架构可以自动终止计时
			if (TS.AutoAlloFree)
				TS.Free = true;
			TS.UserIsr();
		}
		void DoAfterOverflow(TimerState &TS)
		{
			if (!--TS.OverflowLeft)
			{
#ifdef ARDUINO_ARCH_AVR
				TS.Timer.TIMSK = TimerInfo::OCIEA;
#endif
#ifdef ARDUINO_ARCH_SAM
				TcChannel &Channel = TS.Timer.Channel;
				Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_CPCSTOP | TC_CMR_CPCDIS | TC_CMR_TCCLKS_TIMER_CLOCK4;
				// 注意不能覆盖TC_RC，所以只能一个一个赋值
				Channel.TC_IER = TC_IER_CPCS;
				Channel.TC_IDR = ~TC_IDR_CPCS;
#endif
				TS.InterruptServiceRoutine = DoAfterIsr;
			}
		}
		void RepeatIsr(TimerState &TS)
		{
			if (TS.Repeat < InfiniteRepeat && !--TS.Repeat)
			{
#ifdef ARDUINO_ARCH_AVR
				// AVR架构必须手动终止计时
				TS.Timer.TIMSK = 0;
#endif
				// SAM架构可以自动终止计时
				if (TS.AutoAlloFree)
					TS.Free = true;
				TS.UserIsr;
				if (TS.DoneCallback)
					TS.DoneCallback();
			}
			else
				TS.UserIsr;
		}
		void RepeatOverflowIsr(TimerState &TS)
		{
			const TimerInfo &TI = TS.Timer;
#ifdef ARDUINO_ARCH_AVR
			if (TS.Repeat == InfiniteRepeat || --TS.Repeat)
			{
				TI.CtcRegister ^= TI.CtcMask;
				TI.TIMSK = TimerInfo::TOIE;
				TS.OverflowLeft = TS.OverflowCount;
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.UserIsr();
			}
			else
			{
				TI.TIMSK = 0;
				if (TS.AutoAlloFree)
					TS.Free = true;
				TS.UserIsr();
				if (TS.DoneCallback)
					TS.DoneCallback();
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			if (TS.Repeat)
			{
				TcChannel &Channel = TI.Channel;
				Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4;
				Channel.TC_IDR = TC_IDR_CPCS;
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.UserIsr();
			}
			else
			{
				if (TS.AutoAlloFree)
					TS.Free = true;
				TS.UserIsr();
				if (TS.DoneCallback)
					TS.DoneCallback();
			}
#endif
		}
		void RepeatOverflow(TimerState &TS)
		{
			if (!--TS.OverflowLeft)
			{
				const TimerInfo &TI = TS.Timer;
#ifdef ARDUINO_ARCH_AVR
				TI.CtcRegister |= TI.CtcMask;
				TI.TIMSK = TimerInfo::OCIEA;
#endif
#ifdef ARDUINO_ARCH_SAM
				TcChannel &Channel = TI.Channel;
				if (TS.Repeat == InfiniteRepeat || --TS.Repeat)
				{
					Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4;
					TS.OverflowLeft = TS.OverflowCount;
				}
				else
					Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_CPCDIS | TC_CMR_CPCSTOP | TC_CMR_TCCLKS_TIMER_CLOCK4;
				Channel.TC_IER = TC_IER_CPCS;
#endif
			}
			TS.InterruptServiceRoutine = RepeatOverflowIsr;
		}
	}
	void FreeTimer(uint8_t Timer)
	{
		if (Timer < Advanced::NumTimers)
		{
			Advanced::Shutdown(Timer);
			Advanced::TimerStates[Timer].Free = true;
		}
	}
	Exception StartTiming(uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::StartTiming(Timer);
		return E;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency);
		return E;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency);
		return E;
	}
}