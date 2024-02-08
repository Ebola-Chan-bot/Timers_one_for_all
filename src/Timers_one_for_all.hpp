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
		// 记录计时器的硬件常数
		constexpr struct TimerInfo
		{
			static constexpr uint8_t OCIEA = 2;
			static constexpr uint8_t TOIE = 1;
			volatile uint8_t &TCCRB;
			volatile uint8_t &CtcRegister; // 如果需要TCNT达到OCRA时自动清零，需要设置CtcMask位
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
		// 此方法仅关闭计时器功能，并不将其设为可重新分配状态。使用FreeTimer才能让计时器变为可再分配。
		inline void Shutdown(uint8_t Timer)
		{
			Timers[Timer].TIMSK = 0;
		}
#endif
#ifdef ARDUINO_ARCH_SAM
		// 记录计时器的硬件常数
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
		// SAM架构的所有计时器共享4个预分频时钟
		constexpr struct PrescaleClock
		{
			Ticks TicksPerCounter;
			Ticks TicksPerOverflow;
			constexpr PrescaleClock(uint8_t Index) : TicksPerCounter(Ticks(2 << (Index << 1))), TicksPerOverflow(TicksPerCounter * ((uint64_t)1 << 32)) {}
		} PrescaleClocks[4] = {0, 1, 2, 3};
		// 执行任何Advanced方法之前必须先进行全局初始化一次。重复初始化不会出错。
		inline void GlobalInitialize()
		{
			pmc_set_writeprotect(false);
			pmc_enable_all_periph_clk();
		}
		// 使用任何计时器之前必须初始化该计时器一次。重复初始化不会出错。
		void TimerInitialize(uint8_t Timer);
		// 此方法仅关闭计时器功能，并不将其设为可重新分配状态。使用FreeTimer才能让计时器变为可再分配。
		inline void Shutdown(uint8_t Timer)
		{
			Timers[Timer].Channel.TC_CCR = TC_CCR_CLKDIS;
		}
#endif
		constexpr uint8_t NumTimers = std::extent<decltype(Timers)>::value;
		// 记录计时器的运行时状态
		struct TimerState
		{
			void (*InterruptServiceRoutine)(TimerState &State);
			size_t OverflowCount;
			size_t OverflowLeft;
			const TimerInfo &Timer;
			void (*UserIsr)();
			size_t Repeat;
			// 记录计时器是否可被AllocateTimer分配
			bool Free = true;
			// 用户可设定此标志以允许或禁止此计时器被自动分配/释放
			bool AutoAlloFree = true;
		};
		extern TimerState (&TimerStates)[NumTimers];
		// 占用一个空闲的计时器。
		Exception AllocateTimer(uint8_t &Timer);
		void StartTiming(uint8_t Timer);
		template <typename _Rep, typename _Period>
		void GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
		{
			const Advanced::TimerInfo &T = Advanced::Timers[Timer];
			TimeElapsed = std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(Ticks((((uint64_t)Advanced::TimerStates[Timer].OverflowCount <<
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
			Shutdown(Timer);
		}
		void DoAfterIsr(TimerState &TS);
		void DoAfterOverflow(TimerState &TS);
		template <typename _Rep, typename _Period>
		void DoAfter(uint8_t Timer, void (*Do)(), const std::chrono::duration<_Rep, _Period> &After)
		{
			const TimerInfo &T = Timers[Timer];
			uint8_t Clock;
			TimerState &TS = TimerStates[Timer];
#ifdef ARDUINO_ARCH_AVR
			for (Clock = 0; Clock < T.NumPrescaleClocks; ++Clock)
				if (After < T.PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < T.NumPrescaleClocks)
			{
				T.TCCRB = Clock;
				// 只要达到一次OCRA就结束了，所以CTC无所谓，不用设置
				T.ClearCounter();
				T.SetOCRA(After / T.PrescaleClocks[Clock].TicksPerCounter);
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::OCIEA;
				TS.InterruptServiceRoutine = DoAfterIsr;
			}
			else
			{
				Clock = T.NumPrescaleClocks - 1;
				// 计时太长，需要积累几个Overflow
				T.CtcRegister = 0;
				T.TCCRB = Clock;
				// 由于TCCRB和CtcRegister可能是同一个寄存器，上述两行的顺序不能改变
				T.ClearCounter();
				T.SetOCRA((After % T.PrescaleClocks[Clock].TicksPerOverflow).count());
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::TOIE;
				TS.InterruptServiceRoutine = DoAfterOverflow;
				TS.OverflowLeft = After / T.PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			for (Clock = 0; Clock < 4; ++Clock)
				if (After < PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < 4)
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_CPCSTOP | TC_CMR_CPCDIS | Clock, .TC_RC = After / PrescaleClocks[Clock].TicksPerCounter, .TC_IER = TC_IER_CPCS, .TC_IDR = ~TC_IDR_CPCS};
				TS.InterruptServiceRoutine = DoAfterIsr;
			}
			else
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4, .TC_RC = (After % PrescaleClocks[Clock].TicksPerOverflow).count(), .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
				TS.InterruptServiceRoutine = DoAfterOverflow;
				TS.OverflowLeft = After / PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
			TS.UserIsr = Do;
		}
		void UnlimitedRepeatIsr(TimerState &TS);
		void UnlimitedRepeatOverflow(TimerState &TS);
		template <typename _Rep, typename _Period>
		void PeriodicDoRepeat(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Period, void (*Do)())
		{
			const TimerInfo &T = Timers[Timer];
			uint8_t Clock;
			TimerState &TS = TimerStates[Timer];
#ifdef ARDUINO_ARCH_AVR
			for (Clock = 0; Clock < T.NumPrescaleClocks; ++Clock)
				if (Period < T.PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < T.NumPrescaleClocks)
			{
				T.TCCRB = Clock;
				T.CtcRegister |= T.CtcMask; // CtcRegister和TCCRB可能是同一个寄存器，所以用|=防止覆盖
				T.ClearCounter();
				T.SetOCRA(Period / T.PrescaleClocks[Clock].TicksPerCounter);
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::OCIEA;
				TS.InterruptServiceRoutine = UnlimitedRepeatIsr;
			}
			else
			{
				Clock = T.NumPrescaleClocks - 1;
				// 计时太长，需要积累几个Overflow
				T.CtcRegister = 0;
				T.TCCRB = Clock;
				// 由于TCCRB和CtcRegister可能是同一个寄存器，上述两行的顺序不能改变
				T.ClearCounter();
				T.SetOCRA((After % T.PrescaleClocks[Clock].TicksPerOverflow).count());
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::TOIE;
				TS.InterruptServiceRoutine = UnlimitedRepeatOverflow;
				TS.OverflowCount = TS.OverflowLeft = After / T.PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			for (Clock = 0; Clock < 4; ++Clock)
				if (Period < PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < 4)
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | Clock, .TC_RC = Period / PrescaleClocks[Clock].TicksPerCounter, .TC_IER = TC_IER_CPCS, .TC_IDR = ~TC_IDR_CPCS};
				TS.InterruptServiceRoutine = UnlimitedRepeatIsr;
			}
			else
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4, .TC_RC = (Period % PrescaleClocks[Clock].TicksPerOverflow).count(), .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
				TS.InterruptServiceRoutine = UnlimitedRepeatOverflow;
				TS.OverflowCount = Period / PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
			TS.UserIsr = Do;
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
	}
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(void (*Do)(), const std::chrono::duration<_Rep, _Period> &After, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::DoAfter(Timer, Do, After);
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do);
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat);
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat, DoneCallback);
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low);
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat);
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat, DoneCallback);
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
	}
}