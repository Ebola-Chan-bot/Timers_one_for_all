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
#include <Low_level_quick_digital_IO.hpp>
#include <chrono>
#include <vector>
#include <Arduino.h>
namespace Timers_one_for_all
{
	enum class Exception
	{
		Successful_operation,
		Timer_not_exist,
		No_idle_timer,
		AnalogWrite_value_must_between_0_and_1,
		Repeat_is_0,
		Timing_too_short
	};
	// 将Infinite参数设为此值则为无限重复
	constexpr size_t InfiniteRepeat = std::numeric_limits<size_t>::max();
	// Advanced命名空间适用于经验丰富并愿意花时间优化性能的高级用户。普通用户不应使用此命名空间内的方法，因其功能划分粒度较高，稳健性差，需要用户对底层细节有较多了解，不易使用。
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
			size_t OverflowLeft;
			size_t OverflowCount;
			union
			{
				void (*UserIsr)();
				size_t OverflowLeft2;
			};
			const TimerInfo &Timer;
			size_t Repeat;
			std::vector<uint8_t> Pins;
			void (*DoneCallback)();
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
		void Delay(uint8_t Timer, std::chrono::duration<_Rep, _Period> Duration)
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
		Exception DoAfter(uint8_t Timer, void (*Do)(), std::chrono::duration<_Rep, _Period> After)
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
				const size_t OCRA = After / T.PrescaleClocks[Clock].TicksPerCounter;
				if (!OCRA)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				T.TCCRB = Clock;
				// 只要达到一次OCRA就结束了，所以CTC无所谓，不用设置
				T.ClearCounter();
				T.SetOCRA(OCRA);
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
				const size_t TC_RC = After / PrescaleClocks[Clock].TicksPerCounter;
				if (!TC_RC)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_CPCSTOP | TC_CMR_CPCDIS | Clock, .TC_RC = TC_RC, .TC_IER = TC_IER_CPCS, .TC_IDR = ~TC_IDR_CPCS};
				TS.InterruptServiceRoutine = DoAfterIsr;
			}
			else
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4, .TC_RC = (After % PrescaleClocks[Clock].TicksPerOverflow).count(), .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
				TS.InterruptServiceRoutine = DoAfterOverflow;
				TS.OverflowLeft = OverflowLeft;
			}
#endif
			TS.UserIsr = Do;
			return Exception::Successful_operation;
		}
		void RepeatIsr(TimerState &TS);
		void RepeatOverflow(TimerState &TS);
		template <typename _Rep, typename _Period>
		Exception PeriodicDoRepeat(uint8_t Timer, std::chrono::duration<_Rep, _Period> Period, void (*Do)(), size_t Repeat = InfiniteRepeat, void (*DoneCallback)() = nullptr)
		{
			TimerState &TS = TimerStates[Timer];
			const TimerInfo &T = Timers[Timer];
			uint8_t Clock;
#ifdef ARDUINO_ARCH_AVR
			for (Clock = 0; Clock < T.NumPrescaleClocks; ++Clock)
				if (Period < T.PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < T.NumPrescaleClocks)
			{
				const size_t OCRA = Period / T.PrescaleClocks[Clock].TicksPerCounter;
				if (!OCRA)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				T.TCCRB = Clock;
				T.CtcRegister |= T.CtcMask; // CtcRegister和TCCRB可能是同一个寄存器，所以用|=防止覆盖
				T.ClearCounter();
				T.SetOCRA(OCRA);
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::OCIEA;
				TS.InterruptServiceRoutine = RepeatIsr;
			}
			else
			{
				Clock = T.NumPrescaleClocks - 1;
				// 计时太长，需要积累几个Overflow
				T.CtcRegister = 0;
				T.TCCRB = Clock;
				// 由于TCCRB和CtcRegister可能是同一个寄存器，上述两行的顺序不能改变
				T.ClearCounter();
				T.SetOCRA((Period % T.PrescaleClocks[Clock].TicksPerOverflow).count());
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::TOIE;
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.OverflowCount = TS.OverflowLeft = Period / T.PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			for (Clock = 0; Clock < 4; ++Clock)
				if (Period < PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < 4)
			{
				const size_t TC_RC = Period / PrescaleClocks[Clock].TicksPerCounter;
				if (!TC_RC)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | Clock, .TC_RC = TC_RC, .TC_IER = TC_IER_CPCS, .TC_IDR = ~TC_IDR_CPCS};
				TS.InterruptServiceRoutine = RepeatIsr;
			}
			else
			{
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4, .TC_RC = (Period % PrescaleClocks[Clock].TicksPerOverflow).count(), .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.OverflowCount = TS.OverflowLeft = Period / PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
			TS.UserIsr = Do;
			TS.Repeat = Repeat;
			TS.DoneCallback = DoneCallback;
			return Exception::Successful_operation;
		}
		void SquareWaveIsr(TimerState &TS);
		void SquareWaveOverflow(TimerState &TS);
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t Pin, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, size_t Repeat = InfiniteRepeat, void (*DoneCallback)() = nullptr)
		{
			TimerState &TS = TimerStates[Timer];
			const TimerInfo &T = Timers[Timer];
			uint8_t Clock;
			const std::chrono::duration<_Rep, _Period> Cycle = High + Low;
#ifdef ARDUINO_ARCH_AVR
			for (Clock = 0; Clock < T.NumPrescaleClocks; ++Clock)
				if (Period < T.PrescaleClocks[Clock].TicksPerOverflow)
					break;
			if (Clock < T.NumPrescaleClocks)
			{
				const size_t OCRA = Period / T.PrescaleClocks[Clock].TicksPerCounter;
				if (!OCRA)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				T.TCCRB = Clock;
				T.CtcRegister |= T.CtcMask; // CtcRegister和TCCRB可能是同一个寄存器，所以用|=防止覆盖
				T.ClearCounter();
				T.SetOCRA(OCRA);
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::OCIEA;
				TS.InterruptServiceRoutine = RepeatIsr;
			}
			else
			{
				Clock = T.NumPrescaleClocks - 1;
				// 计时太长，需要积累几个Overflow
				T.CtcRegister = 0;
				T.TCCRB = Clock;
				// 由于TCCRB和CtcRegister可能是同一个寄存器，上述两行的顺序不能改变
				T.ClearCounter();
				T.SetOCRA((Period % T.PrescaleClocks[Clock].TicksPerOverflow).count());
				T.TIFR = UINT8_MAX;
				T.TIMSK = TimerInfo::TOIE;
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.OverflowCount = TS.OverflowLeft = Period / T.PrescaleClocks[Clock].TicksPerOverflow;
			}
#endif
#ifdef ARDUINO_ARCH_SAM
			for (Clock = 0; Clock < 4; ++Clock)
				if (Cycle < PrescaleClocks[Clock].TicksPerOverflow)
					break;
			const PrescaleClock &PC = PrescaleClocks[Clock];
			if (Clock < 4)
			{
				const size_t TC_RA = (Low_level_quick_digital_IO::DigitalRead<OUTPUT>(Pin) ? High : Low) / PC.TicksPerCounter;
				if (!TC_RA)
				{
					if (TS.AutoAlloFree)
						TS.Free = true;
					return Exception::Timing_too_short;
				}
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | Clock, .TC_RA = TC_RA, .TC_RC = Cycle / PC.TicksPerCounter, .TC_IER = TC_IER_CPCS | TC_IER_CPAS, .TC_IDR = ~(TC_IDR_CPCS | TC_IDR_CPAS)};
				TS.InterruptServiceRoutine = SquareWaveIsr;
			}
			else
			{
				const std::chrono::duration<_Rep, _Period> TC_RA = Low_level_quick_digital_IO::DigitalRead<OUTPUT>(Pin) ? High : Low;
				NVIC_ClearPendingIRQ(T.irq);
				T.Channel = {.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG, .TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK4, .TC_RA = (TC_RA % PC.TicksPerOverflow).count(), .TC_RC = (Cycle % PC.TicksPerOverflow).count(), .TC_IER = TC_IER_COVFS, .TC_IDR = ~TC_IDR_COVFS};
				TS.InterruptServiceRoutine = RepeatOverflow;
				TS.OverflowCount = 0;
				TS.OverflowLeft = TC_RA / PC.TicksPerOverflow;
				TS.OverflowLeft2 = Cycle / PC.TicksPerOverflow;
				TS.InterruptServiceRoutine = SquareWaveOverflow;
			}
#endif
			TS.Pins.assign({Pin});
			TS.Repeat = Repeat;
			TS.DoneCallback = DoneCallback;
			return Exception::Successful_operation;
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, size_t Repeat)
		{
		}
		template <typename _Rep, typename _Period>
		Exception SquareWave(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, size_t Repeat, void (*DoneCallback)())
		{
		}
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency);
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration)
		{
		}
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t Pin, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, void (*DoneCallback)())
		{
		}
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency);
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration)
		{
		}
		template <typename _Rep, typename _Period>
		Exception Tone(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, void (*DoneCallback)())
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
	// 此方法返回的Timer需要手动FreeTimer
	Exception StartTiming(uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
	{
		if (Timer > Advanced::NumTimers)
			return Exception::Timer_not_exist;
		Advanced::GetTiming(Timer, TimeElapsed);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception Delay(std::chrono::duration<_Rep, _Period> Duration)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Delay(Duration);
		return E;
	}
	// 此方法返回的Timer在任务完成后自动释放。你也可以在预定时间之前提前手动释放，则任务取消。
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(void (*Do)(), std::chrono::duration<_Rep, _Period> After, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::DoAfter(Timer, Do, After);
		return E;
	}
	// 如果指定了有限的重复次数（包括0次），此方法返回的Timer在所有重复结束后自动释放。你也可以在预定时间之前提前手动释放，则后续任务取消。第1次重复也需要等待Period时间才会执行。
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(std::chrono::duration<_Rep, _Period> Period, void (*Do)(), uint8_t &Timer, size_t Repeat = Advanced::TimerState::InfiniteRepeat, void (*DoneCallback)() = nullptr)
	{
		if (Repeat)
		{
			TOFA_AllocateTimerStuff;
			E = Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat, DoneCallback);
			return E;
		}
		else
		{
			DoneCallback();
			return Exception::Repeat_is_0;
		}
	}
	// 此处的Repeat参数是电平反转的次数，即1代表电平反转一次，2才是一个完整的周期。注意第1次反转也需要等待High或Low时间后才发生，取决于当前电平状态。
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t Pin, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, uint8_t &Timer, size_t Repeat = InfiniteRepeat, void (*DoneCallback)())
	{
		TOFA_AllocateTimerStuff;
		pinMode(Pin, OUTPUT);
		E = dvanced::SquareWave(Timer, Pin, High, Low, Repeat, DoneCallback);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, std::chrono::duration<_Rep, _Period> High, std::chrono::duration<_Rep, _Period> Low, size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat, DoneCallback);
		return E;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		return E;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		return E;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, std::chrono::duration<_Rep, _Period> Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		return E;
	}
}