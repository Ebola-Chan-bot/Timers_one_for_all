#pragma once
#include "HardwareTimers.hpp"
#include <chrono>
#include <functional>
#include <dynarray>
#include <Arduino.h>
namespace Timers_one_for_all
{
	constexpr uint8_t HardwareTimers[] = {
#ifdef TOFA_USE_TIMER0
		0,
#endif
#ifdef TOFA_USE_TIMER1
		1,
#endif
#ifdef TOFA_USE_TIMER2
		2,
#endif
#ifdef TOFA_USE_TIMER3
		3,
#endif
#ifdef TOFA_USE_TIMER4
		4,
#endif
#ifdef TOFA_USE_TIMER5
		5,
#endif
#ifdef TOFA_USE_TIMER6
		6,
#endif
#ifdef TOFA_USE_TIMER7
		7,
#endif
#ifdef TOFA_USE_TIMER8
		8,
#endif
	};
	constexpr uint8_t NumTimers = std::extent<decltype(HardwareTimers)>::value;
	struct TimerInterrupt
	{
		std::function<void()> Overflow;
		std::function<void()> CompareA;
		std::function<void()> CompareB;
	};
	extern TimerInterrupt TimerInterrupts[NumTimers];
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	extern bool TimerFree[NumTimers];
	// 返回值是计时器的软件号，范围[0,NumTimers)。使用HardwareTimers以将软件号映射为硬件号。如果没有空闲的计时器，返回NumTimers。
	uint8_t AllocateSoftwareTimer();
	// 将指定软件号的计时器标记为空闲，使其可以被再分配，但不会停止任何执行中的进程。输入已空闲的软件号不做任何事。输入大于等于NumTimers的无效软件号是未定义行为。
	inline void FreeSoftwareTimer(uint8_t SoftwareIndex) { TimerFree[SoftwareIndex] = true; }
#ifdef ARDUINO_ARCH_AVR
	struct CommonPrescalers
	{
		static constexpr uint16_t Prescalers[] = {1, 8, 64, 256, 1024};
	};
	template <typename TCounter>
	struct AvrTimer
	{
		volatile uint8_t &TCCRA;
		volatile uint8_t &TCCRB;
		volatile uint8_t &TIFR;
		volatile uint8_t &TIMSK;
		uint8_t CtcMask;
		volatile TCounter &TCNT;
		volatile TCounter &OCRA;
		volatile TCounter &OCRB;
	};
	struct HardwareTimer1 : public AvrTimer<uint16_t>, public CommonPrescalers
	{
	};
	// 必须使用此模板的特化版本，非特化版本不允许实例化
	template <uint8_t HardwareIndex>
	constexpr auto HardwareTimer = []()
	{
		static_assert(HardwareIndex != HardwareIndex, "不可用的计时器号");
		return 0;
	}();
#ifdef TOFA_USE_TIMER0
	struct HardwareTimer0 : public AvrTimer<uint8_t>, public CommonPrescalers
	{
	};
	template <>
	constexpr HardwareTimer0 HardwareTimer<0>{TCCR0A, TCCR0B, TIFR0, TIMSK0, WGM01, TCNT0, OCR0A, OCR0B};
#endif
#ifdef TOFA_USE_TIMER1
	template <>
	constexpr HardwareTimer1 HardwareTimer<1>{TCCR1A, TCCR1B, TIFR1, TIMSK1, WGM12, TCNT1, OCR1A, OCR1B};
#endif
#ifdef TOFA_USE_TIMER2
	struct HardwareTimer2 : public AvrTimer<uint8_t>
	{
		static constexpr uint16_t Prescalers[] = {1, 8, 32, 64, 128, 256, 1024};
	};
	template <>
	constexpr HardwareTimer2 HardwareTimer<2>{TCCR2A, TCCR2B, TIFR2, TIMSK2, WGM21, TCNT2, OCR2A, OCR2B};
#endif
#ifdef TOFA_USE_TIMER3
	template <>
	constexpr HardwareTimer1 HardwareTimer<3>{TCCR3A, TCCR3B, TIFR3, TIMSK3, WGM32, TCNT3, OCR3A, OCR3B};
#endif
#ifdef TOFA_USE_TIMER4
	template <>
	constexpr HardwareTimer1 HardwareTimer<4>{TCCR4A, TCCR4B, TIFR4, TIMSK4, WGM42, TCNT4, OCR4A, OCR4B};
#endif
#ifdef TOFA_USE_TIMER5
	template <>
	constexpr HardwareTimer1 HardwareTimer<5>{TCCR5A, TCCR5B, TIFR5, TIMSK5, WGM52, TCNT5, OCR5A, OCR5B};
#endif
#endif
}