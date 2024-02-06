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
#ifdef ARDUINO_ARCH_AVR
		constexpr struct TimerInfo
		{
			volatile uint8_t &TCCRA;
			volatile uint8_t &TCCRB;
			volatile void *TCNT;
			volatile uint8_t &TIFR;
			volatile uint8_t &TIMSK;
			uint8_t CounterBytes;
			uint8_t NumPrescaleDivisors;
			uint16_t PrescaleDivisors[7];
			uint16_t ReadCounter() const;
			void SetCounter(uint16_t Counter) const;
		} Timers[] = {
#ifndef TOFA_DISABLE_0
			{TCCR0A, TCCR0B, &TCNT0, TIFR0, TIMSK0, 1, 5, {1, 8, 64, 256, 1024}},
#endif
#ifndef TOFA_DISABLE_1
			{TCCR1A, TCCR1B, &TCNT1, TIFR1, TIMSK1, 2, 5, {1, 8, 64, 256, 1024}},
#endif
#ifndef TOFA_DISABLE_2
			{TCCR2A, TCCR2B, &TCNT2, TIFR2, TIMSK2, 1, 7, {1, 8, 32, 64, 128, 256, 1024}},
#endif
#ifdef __AVR_ATmega2560__
#ifndef TOFA_DISABLE_3
			{TCCR3A, TCCR3B, &TCNT3, TIFR3, TIMSK3, 2, 5, {1, 8, 64, 256, 1024}},
#endif
#ifndef TOFA_DISABLE_4
			{TCCR4A, TCCR4B, &TCNT4, TIFR4, TIMSK4, 2, 5, {1, 8, 64, 256, 1024}},
#endif
#ifndef TOFA_DISABLE_5
			{TCCR5A, TCCR5B, &TCNT5, TIFR5, TIMSK5, 2, 5, {1, 8, 64, 256, 1024}}
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
			Tc *tc;
			uint32_t channel;
			IRQn_Type irq;
			uint32_t ReadCounter() const
			{
				return TC_ReadCV(tc, channel);
			}
			void SetCounter(uint32_t Counter) const
			{
				tc->TC_CHANNEL[channel].TC_CV = Counter;
			}
		} Timers[] = {
#ifndef TOFA_DISABLE_0
			{TC0, 0, TC0_IRQn},
#endif
#ifndef TOFA_DISABLE_1
			{TC0, 1, TC1_IRQn},
#endif
#ifndef TOFA_DISABLE_2
			{TC0, 2, TC2_IRQn},
#endif
#ifndef TOFA_DISABLE_3
			{TC1, 0, TC3_IRQn},
#endif
#ifndef TOFA_DISABLE_4
			{TC1, 1, TC4_IRQn},
#endif
#ifndef TOFA_DISABLE_5
			{TC1, 2, TC5_IRQn},
#endif
#ifndef TOFA_DISABLE_6
			{TC2, 0, TC6_IRQn},
#endif
#ifndef TOFA_DISABLE_7
			{TC2, 1, TC7_IRQn},
#endif
#ifndef TOFA_DISABLE_8
			{TC2, 2, TC8_IRQn},
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
			TC_Stop(Timers[Timer].tc, Timers[Timer].channel);
		}
#endif
		constexpr uint8_t NumTimers = std::extent<decltype(Timers)>::value;
		// 占用一个空闲的计时器。
		Exception AllocateTimer(uint8_t &Timer);
		Exception StartTiming(uint8_t Timer);
		template <typename _Rep, typename _Period>
		Exception Delay(uint8_t Timer, const std::chrono::duration<_Rep, _Period> &Duration)
		{
		}
		template <typename _Rep, typename _Period>
		Exception DoAfter(uint8_t Timer, void (*Do)(), const std::chrono::duration<_Rep, _Period> &After)
		{
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
#define TOFA_AllocateTimerStuff                   \
	Exception E = Advanced::AllocateTimer(Timer); \
	if ((bool)E)                                  \
		return E;
#endif
#ifdef ARDUINO_ARCH_SAM
#define TOFA_AllocateTimerStuff                   \
	Exception E = Advanced::AllocateTimer(Timer); \
	if ((bool)E)                                  \
		return E;                                 \
	Advanced::GlobalInitialize();                 \
	Advanced::TimerInitialize(Timer);
#endif
#define TOFA_FreeTimerStuff \
	if ((bool)E)            \
		FreeTimer(Timer);   \
	return E;
	Exception StartTiming(uint8_t &Timer);
	template <typename _Rep, typename _Period>
	Exception GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
	{
	}
	template <typename _Rep, typename _Period>
	Exception Delay(const std::chrono::duration<_Rep, _Period> &Duration)
	{
		uint8_t Timer;
		TOFA_AllocateTimerStuff;
		E = Advanced::Delay(Timer, Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception DoAfter(void (*Do)(), const std::chrono::duration<_Rep, _Period> &After, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::DoAfter(Timer, Do, After);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::PeriodicDoRepeat(Timer, Period, Do);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception PeriodicDoRepeat(const std::chrono::duration<_Rep, _Period> &Period, void (*Do)(), size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::PeriodicDoRepeat(Timer, Period, Do, Repeat, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::SquareWave(Timer, NumPins, Pins, High, Low);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception SquareWave(uint8_t NumPins, const uint8_t *Pins, const std::chrono::duration<_Rep, _Period> &High, const std::chrono::duration<_Rep, _Period> &Low, size_t Repeat, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::SquareWave(Timer, NumPins, Pins, High, Low, Repeat, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		TOFA_FreeTimerStuff;
	}
	Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer);
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, const std::chrono::duration<_Rep, _Period> &Duration, void (*DoneCallback)(), uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency, Duration, DoneCallback);
		TOFA_FreeTimerStuff;
	}
}