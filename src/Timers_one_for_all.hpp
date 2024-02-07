#pragma once
// 如需禁用特定的硬件计时器，请取消注释以下行，或者在包含本头文件之前先添加以下定义，以解决和其它库的冲突问题。对于AVR架构，建议始终禁用计时器0。
// #define TOFA_DISABLE_0
// #define TOFA_DISABLE_1
// #define TOFA_DISABLE_2
// #define TOFA_DISABLE_3
// #define TOFA_DISABLE_4
// #define TOFA_DISABLE_5
// #define TOFA_DISABLE_6
// #define TOFA_DISABLE_7
// #define TOFA_DISABLE_8
// 被禁用的硬件计时器之后的计时器，其在本库中的编号会依次前移。在这种情况下，硬件计时器号与软件计时器号将不再一一对应。例如，如果禁用了计时器4，则硬件计时器5~8将对应软件计时器4~7。
#include <Arduino.h>
#include <chrono>
namespace Timers_one_for_all
{
	enum class Exception
	{
		Successful_operation,
		Timer_not_exist,
		No_idle_timer,
		AnalogWrite_value_must_between_0_and_1,
	};
	// 适用于希望追求高性能的高级用户
	namespace Advanced
	{
		using Ticks = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
#ifdef ARDUINO_ARCH_AVR
		constexpr struct TimerInfo
		{
			static constexpr uint8_t OCIEA = 2;
			static constexpr uint8_t TOIE = 1;
			volatile uint8_t &TCCRB;
			volatile uint8_t &CtcRegister;
			volatile void *TCNT;
			volatile void *OCRA;
			volatile uint8_t &TIFR;
			volatile uint8_t &TIMSK;
			uint8_t CtcMask;
			uint8_t CounterBits;
			uint8_t NumPrescaleClocks;
			struct
			{
				uint16_t Divisor;
				Ticks TicksPerCounter;
				Ticks TicksPerOverflow;
			} PrescaleClocks[7];
			uint16_t ReadCounter() const;
			void ClearCounter() const;
			void SetOCRA(uint16_t OCRA) const;
			template <typename... T>
			constexpr TimerInfo(volatile uint8_t &TCCRB, volatile uint8_t &CtcRegister, volatile void *TCNT, volatile void *OCRA, volatile uint8_t &TIFR, volatile uint8_t &TIMSK, uint8_t CtcBit, uint8_t CounterBits, T... PrescaleDivisors) : TCCRB(TCCRB), CtcRegister(CtcRegister), TCNT(TCNT), OCRA(OCRA), TIFR(TIFR), TIMSK(TIMSK), CtcMask(1 << CtcBit), CounterBits(CounterBits), NumPrescaleClocks(sizeof...(PrescaleDivisors)), PrescaleClocks{{PrescaleDivisors, Ticks(PrescaleDivisors), Ticks(PrescaleDivisors << CounterBits)}...} {}
		} Timers[] = {
#ifndef TOFA_DISABLE_0
			{TCCR0B, TCCR0A, &TCNT0, &OCR0A, TIFR0, TIMSK0, WGM01, 8, 1, 8, 64, 256, 1024},
#endif
#ifndef TOFA_DISABLE_1
			{TCCR1B, TCCR1B, &TCNT1, &OCR1A, TIFR1, TIMSK1, WGM12, 16, 1, 8, 64, 256, 1024},
#endif
#ifndef TOFA_DISABLE_2
			{TCCR2B, TCCR2A, &TCNT2, &OCR2A, TIFR2, TIMSK2, WGM21, 8, 1, 8, 32, 64, 128, 256, 1024},
#endif
#ifdef __AVR_ATmega2560__
#ifndef TOFA_DISABLE_3
			{TCCR3B, TCCR3B, &TCNT3, &OCR3A, TIFR3, TIMSK3, WGM32, 16, 1, 8, 64, 256, 1024},
#endif
#ifndef TOFA_DISABLE_4
			{TCCR4B, TCCR4B, &TCNT4, &OCR4A, TIFR4, TIMSK4, WGM42, 16, 1, 8, 64, 256, 1024},
#endif
#ifndef TOFA_DISABLE_5
			{TCCR5B, TCCR5B, &TCNT5, &OCR5A, TIFR5, TIMSK5, WGM52, 16, 1, 8, 64, 256, 1024}
#endif
#endif
		};
		inline void Shutdown(uint8_t Timer)
		{
			Timers[Timer].TIMSK = 0;
		}
#endif
#ifdef ARDUINO_ARCH_SAM
		constexpr struct TimerInfo
		{
			TcChannel &Channel;
			IRQn_Type irq;
		} Timers[] = {
#ifndef TOFA_DISABLE_0
			{TC0->TC_CHANNEL[0], TC0_IRQn},
#endif
#ifndef TOFA_DISABLE_1
			{TC0->TC_CHANNEL[1], TC1_IRQn},
#endif
#ifndef TOFA_DISABLE_2
			{TC0->TC_CHANNEL[2], TC2_IRQn},
#endif
#ifndef TOFA_DISABLE_3
			{TC1->TC_CHANNEL[0], TC3_IRQn},
#endif
#ifndef TOFA_DISABLE_4
			{TC1->TC_CHANNEL[1], TC4_IRQn},
#endif
#ifndef TOFA_DISABLE_5
			{TC1->TC_CHANNEL[2], TC5_IRQn},
#endif
#ifndef TOFA_DISABLE_6
			{TC2->TC_CHANNEL[0], TC6_IRQn},
#endif
#ifndef TOFA_DISABLE_7
			{TC2->TC_CHANNEL[1], TC7_IRQn},
#endif
#ifndef TOFA_DISABLE_8
			{TC2->TC_CHANNEL[2], TC8_IRQn},
#endif
		};
		// 执行任何Advanced方法之前必须先进行全局初始化一次。重复初始化不会出错。
		inline void GlobalInitialize()
		{
			pmc_set_writeprotect(false);
			pmc_enable_all_periph_clk();
		}
		// 使用任何计时器之前必须初始化该计时器一次。重复初始化不会出错。
		void TimerInitialize(uint8_t Timer);
		inline void Shutdown(uint8_t Timer)
		{
			Timers[Timer].Channel.TC_CCR = TC_CCR_CLKDIS;
		}
#endif
		constexpr uint8_t NumTimers = std::extent<decltype(Timers)>::value;
		// 占用一个空闲的计时器。
		Exception AllocateTimer(uint8_t &Timer);
		void StartTiming(uint8_t Timer);
		struct TimerState
		{
			void (*InterruptServiceRoutine)(TimerState &State);
			size_t OverflowCount;
			union
			{
					const Timers_one_for_all::Advanced::TimerInfo *Timer;
					struct 
					{
						void(*UserIsr)();
						size_t Repeat;
					}
			} Arguments;

		} TimerStates[Timers_one_for_all::Advanced::NumTimers];
		template <typename _Rep, typename _Period>
		void GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
		{
			const Advanced::TimerInfo &T = Advanced::Timers[Timer];
			TimeElapsed = std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(Ticks((((uint64_t)Advanced::TimerStates[Timer].Arguments.TimingState.OverflowCount <<
#ifdef ARDUINO_ARCH_AVR
			T.CounterBits) + T.ReadCounter()) * T.PrescaleClocks[T.TCCRB].TicksPerCounter
#endif
#ifdef ARDUINO_ARCH_SAM
			32) + T.Channel.TC_CV << ((T.Channel.TC_CMR & 0b11) << 1) + 1)
#endif
		 	));
		}
		template <typename _Rep, typename _Period>
		void Delay(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Duration)
		{
			StartTiming(Timer);
			std::chrono::duration<_Rep, _Period> TimeElapsed;
			do
				GetTiming(Timer, TimeElapsed);
			while (TimeElapsed < Duration);
		}
		template <typename _Rep, typename _Period>
		Exception DoAfter(uint8_t Timer, void (*Do)(), const std::chrono::duration<_Rep, _Period> &After)
		{
#ifdef ARDUINO_ARCH_AVR
			const TimerInfo &T = Timers[Timer];
			uint8_t Clock;
			for (Clock = 0; Clock < T.NumPrescaleClocks; ++Clock)
				if (After < T.PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < T.NumPrescaleClocks)
			{
				T.TCCRB = Clock;
				T.CtcRegister |= T.CtcMask;
				T.ClearCounter();
				T.SetOCRA(After / T.PrescaleClocks[Clock].TicksPerCounter);
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::OCIEA;
				TimerStates[Timer] = {.InterruptServiceRoutine = [](TimerState &TS)
									  { TS.Arguments.UserIsr(); },
									  .Arguments = {.} }
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			constexpr uint64_t BaseMultiplier = (uint64_t)1000000000 << 33;
			constexpr std::chrono::nanoseconds(&ClockLimits)[] = TicksToNanos<BaseMultiplier, BaseMultiplier << 2, BaseMultiplier << 4, BaseMultiplier << 6>::value;
			uint8_t Clock;
			for (Clock = 0; Clock < 4; ++Clock)
				if (After < ClockLimits[Clock])
					break;
			if (Clock < 4)
			{
				TimerStates[Timer].U
			}
#endif
		}
		template <typename _Rep, typename _Period>
		Exception PeriodicDoRepeat(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Period, void (*Do)())
		{
		}
		template <typename _Rep, typename _Period>
		Exception PeriodicDoRepeat(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat)
		{
		}
		template <typename _Rep, typename _Period>
		Exception PeriodicDoRepeat(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, void (*DoneCallback)())
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t Pin, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t Pin, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t Pin, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, void (*DoneCallback)())
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, void (*DoneCallback)())
		{
		}
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency);
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration)
		{
		}
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)())
		{
		}
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency);
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration)
		{
		}
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)())
		{
		}
		// 记录计时器是否忙碌的数组。如果混合使用本层和更低级的API，用户应当负责管理此数组，否则可能造成调度错误。
		volatile bool TimerBusy[NumTimers] = {false};
	}

	// 停止并释放一个忙碌的计时器，额外检查计时器号是否正确。
	void FreeTimer(uint8_t Timer);
#ifdef ARDUINO_ARCH_AVR
#define TOFA_AllocateTimerStuff                         \
	const Exception E = Advanced::AllocateTimer(Timer); \
	if ((bool)E)                                        \
		return E;
#endif
#ifdef ARDUINO_ARCH_SAM
#define TOFA_AllocateTimerStuff                         \
	const Exception E = Advanced::AllocateTimer(Timer); \
	if ((bool)E)                                        \
		return E;                                       \
	Advanced::GlobalInitialize();                       \
	Advanced::TimerInitialize(Timer);
#endif
#define TOFA_FreeTimerStuff \
	FreeTimer(Timer);       \
	return E;
	Exception StartTiming(uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
	{
		if (Timer > Advanced::NumTimers)
			return Exception::Timer_not_exist;
		Advanced::GetTiming(Timer, TimeElapsed);
	}
	template <typename _Rep, typename _Period>
	inline Exception Delay(const std::chrono::duration<_Rep, _Period> &Duration)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Delay(Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(void (*Do)(), const std::chrono::duration<_Rep, _Period> &After, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::DoAfter(Timer, Do, After);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		TOFA_FreeTimerStuff;
	}
}