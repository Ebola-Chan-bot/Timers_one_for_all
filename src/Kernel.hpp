#pragma once
#include <chrono>
#include <functional>
#include <dynarray>
#include <Arduino.h>
namespace Timers_one_for_all
{
	struct SoftwareTimer
	{
		uint8_t HardwareIndex;
		bool Free;
		constexpr SoftwareTimer(uint8_t HardwareIndex)
			: HardwareIndex(HardwareIndex), Free(true) {}
		SoftwareTimer() {}
		SoftwareTimer(const SoftwareTimer &) = delete;
	};
	// 可调度的硬件计时器列表。用此数组可将计时器软件号映射到硬件号。
	extern std::dynarray<SoftwareTimer> SchedulableTimers;
#ifdef ARDUINO_ARCH_AVR
	constexpr uint8_t NumHardwareTimers = 6;
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
	struct HardwareTimer0 : public AvrTimer<uint8_t>, public CommonPrescalers
	{
	};
	struct HardwareTimer1 : public AvrTimer<uint16_t>, public CommonPrescalers
	{
	};
	struct HardwareTimer2 : public AvrTimer<uint8_t>
	{
		static constexpr uint16_t Prescalers[] = {1, 8, 32, 64, 128, 256, 1024};
	};
	// 必须使用此模板的特化版本，非特化版本不允许实例化
	template <uint8_t HardwareIndex>
	constexpr auto HardwareTimer = []()
	{
		static_assert(HardwareIndex != HardwareIndex, "不可用的计时器号");
		return 0;
	}();
	template <>
	constexpr HardwareTimer0 HardwareTimer<0>{TCCR0A, TCCR0B, TIFR0, TIMSK0, WGM01, TCNT0, OCR0A, OCR0B};
	template <>
	constexpr HardwareTimer1 HardwareTimer<1>{TCCR1A, TCCR1B, TIFR1, TIMSK1, WGM12, TCNT1, OCR1A, OCR1B};
	template <>
	constexpr HardwareTimer2 HardwareTimer<2>{TCCR2A, TCCR2B, TIFR2, TIMSK2, WGM21, TCNT2, OCR2A, OCR2B};
	template <>
	constexpr HardwareTimer1 HardwareTimer<3>{TCCR3A, TCCR3B, TIFR3, TIMSK3, WGM32, TCNT3, OCR3A, OCR3B};
	template <>
	constexpr HardwareTimer1 HardwareTimer<4>{TCCR4A, TCCR4B, TIFR4, TIMSK4, WGM42, TCNT4, OCR4A, OCR4B};
	template <>
	constexpr HardwareTimer1 HardwareTimer<5>{TCCR5A, TCCR5B, TIFR5, TIMSK5, WGM52, TCNT5, OCR5A, OCR5B};
#endif
	struct TimerInterrupt
	{
		std::function<void()> Overflow;
		std::function<void()> CompareA;
		std::function<void()> CompareB;
	};
	extern TimerInterrupt TimerInterrupts[NumHardwareTimers];
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
}