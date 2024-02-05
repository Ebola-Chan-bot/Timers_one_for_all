#pragma once
// 如需禁用特定的计时器，请取消注释以下行，以解决和其它库的冲突问题
// #define TOFA_DISABLE_0
// #define TOFA_DISABLE_1
// #define TOFA_DISABLE_2
// #define TOFA_DISABLE_3
// #define TOFA_DISABLE_4
// #define TOFA_DISABLE_5
// #define TOFA_DISABLE_6
// #define TOFA_DISABLE_7
// #define TOFA_DISABLE_8
#include <Arduino.h>
#include <chrono>
namespace Timers_one_for_all
{
	enum class Exception
	{
		Successful_operation,
		Timer_not_exist,
		No_idle_timer
	};

// API等级0：底层硬件细节，追求极致优化，0代价抽象，但使用复杂，需要对硬件有一定了解
#ifdef ARDUINO_ARCH_AVR
#ifdef __AVR_ATmega2560__
	constexpr uint8_t NumTimers = 6;
#else
	constexpr uint8_t NumTimers = 3;
#endif
#endif
#ifdef ARDUINO_ARCH_SAM
	constexpr uint8_t NumTimers = 9;
#endif
	constexpr struct
	{
		uint64_t CounterMax;
		uint8_t NumPrescaleDivisors;
		uint16_t PrescaleDivisors[7];
	} TimerInfo[] =
		{
#ifdef ARDUINO_ARCH_AVR
			{UINT8_MAX + 1, 5, {1, 8, 64, 256, 1024}},
			{UINT16_MAX + 1, 5, {1, 8, 64, 256, 1024}},
			{UINT8_MAX + 1, 7, {1, 8, 32, 64, 128, 256, 1024}},
#ifdef __AVR_ATmega2560__
			{UINT16_MAX + 1, 5, {1, 8, 64, 256, 1024}},
			{UINT16_MAX + 1, 5, {1, 8, 64, 256, 1024}},
			{UINT16_MAX + 1, 5, {1, 8, 64, 256, 1024}},
#endif
#endif
#ifdef __SAM3X8E__
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
			{UINT32_MAX + 1, 4, {2, 8, 32, 128}},
#endif
	};
	//适用于希望追求高性能的高级用户
	namespace Advanced
	{
		void Shutdown(uint8_t Timer);
		Exception AnalogWrite(uint8_t Timer, uint8_t Pin, float Value);
		Exception AnalogWrite(uint8_t Timer, uint8_t NumPins, const uint8_t *Pins, float Value);
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
		// 占用一个空闲的计时器。
		Exception AllocateTimer(uint8_t &Timer);
	}

	// 停止并释放一个忙碌的计时器，额外检查计时器号是否正确。
	inline void FreeTimer(uint8_t Timer)
	{
		if (Timer < NumTimers)
		{
			Advanced::Shutdown(Timer);
			Advanced::TimerBusy[Timer] = false;
		}
	}
#define TOFA_AllocateTimerStuff                   \
	Exception E = Advanced::AllocateTimer(Timer); \
	if ((bool)E)                                  \
		return E;
#define TOFA_FreeTimerStuff \
	if ((bool)E)            \
		FreeTimer(Timer);   \
	return E;
	inline Exception AnalogWrite(uint8_t Pin, float Value, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::AnalogWrite(Timer, Pin, Value);
		TOFA_FreeTimerStuff;
	}
	inline Exception AnalogWrite(uint8_t NumPins, const uint8_t *Pins, float Value, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::AnalogWrite(Timer, NumPins, Pins, Value);
		TOFA_FreeTimerStuff;
	}
	inline Exception StartTiming(uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::StartTiming(Timer);
		TOFA_FreeTimerStuff;
	}
	template <typename _Rep, typename _Period>
	Exception GetTiming(uint8_t Timer, std::chrono::duration<_Rep, _Period> &TimeElapsed)
	{
	}
	template <typename _Rep, typename _Period>
	Exception Delay(const std::chrono::duration<_Rep, _Period> &Duration)
	{
		uint8_t Timer;
		Exception E = StartTiming(Timer);
		if ((bool)E)
			return E;
		std::chrono::duration<_Rep, _Period> TimeElapsed;
		do
		{
			E = GetTiming(Timer, TimeElapsed);
			if ((bool)E)
			{
				Advanced::Shutdown(Timer);
				Advanced::TimerBusy[Timer] = false;
				break;
			}
		} while (TimeElapsed < Duration);
		return E;
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
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
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
	inline Exception Tone(uint8_t NumPins, const uint8_t *Pins, uint16_t Frequency, uint8_t &Timer)
	{
		TOFA_AllocateTimerStuff;
		E = Advanced::Tone(Timer, NumPins, Pins, Frequency);
		TOFA_FreeTimerStuff;
	}
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