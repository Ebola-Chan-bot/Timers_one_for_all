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
		void RepeatIsr(TimerState &TS)
		{
			switch (TS.OverflowLeft)
			{
			case 0:
				if (TS.Repeat == InfiniteRepeat || --TS.Repeat)
				{
					if (TS.OverflowLeft = TS.OverflowCount) // 这里就是要赋值并判断非零，不是判断相等
#ifdef ARDUINO_ARCH_AVR
						TS.Timer.CtcRegister &= ~TS.Timer.CtcMask;
#endif
#ifdef ARDUINO_ARCH_SAM
					TS.Timer.Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4;
#endif
					TS.UserIsr();
				}
				else
				{
#ifdef ARDUINO_ARCH_AVR
					TS.Timer.TIMSK = 0;
#endif
					if (TS.AutoAlloFree)
						TS.Free = true;
					TS.UserIsr();
					if (TS.DoneCallback)
						TS.DoneCallback();
				}
				break;
			case 1:
#ifdef ARDUINO_ARCH_AVR
				TS.Timer.CtcRegister |= TS.Timer.CtcMask;
#endif
#ifdef ARDUINO_ARCH_SAM
				TS.Timer.Channel.TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | (TS.Repeat == 1 ? TC_CMR_CPCDIS | TC_CMR_CPCSTOP : 0) | TC_CMR_TCCLKS_TIMER_CLOCK4;
#endif
			default:
				TS.OverflowLeft--;
			}
		}
		void SquareWaveIsr(TimerState &TS)
		{
			if (TS.Repeat == InfiniteRepeat || --TS.Repeat)
				Low_level_quick_digital_IO::DigitalToggle(TS.Pin);
			else
			{
#ifdef ARDUINO_ARCH_AVR
				TS.Timer.TIMSK = 0;
#endif
#ifdef ARDUINO_ARCH_SAM
				TS.Timer.Channel.TC_CCR = TC_CCR_CLKDIS;
#endif
				if (TS.AutoAlloFree)
					TS.Free = true;
				Low_level_quick_digital_IO::DigitalToggle(TS.Pin);
				if (TS.DoneCallback)
					TS.DoneCallback();
			}
		}
		void SquareWaveOverflow(TimerState &TS)
		{
			if (TS.Repeat == InfiniteRepeat || --TS.Repeat)
			{
#ifdef ARDUINO_ARCH_SAM
				TcChannel &Channel = TS.Timer.Channel;
				if ((Channel.TC_IDR = Channel.TC_IMR) == TC_IMR_CPAS)
				{
					Channel.TC_IER = TC_IER_CPCS;
				}
				else
				{
					Channel.TC_IER = TC_IER_CPAS;
				}
#endif
				if (TS.OverflowLeft2)
					TS.OverflowCount++;
				Low_level_quick_digital_IO::DigitalToggle(TS.Pin);
			}
			else
			{
#ifdef ARDUINO_ARCH_AVR
				TS.Timer.TIMSK = 0;
#endif
#ifdef ARDUINO_ARCH_SAM
				TS.Timer.Channel.TC_CCR = TC_CCR_CLKDIS;
#endif
				if (TS.AutoAlloFree)
					TS.Free = true;
				Low_level_quick_digital_IO::DigitalToggle(TS.Pin);
				if (TS.DoneCallback)
					TS.DoneCallback();
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