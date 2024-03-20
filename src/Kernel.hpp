#pragma once
#include "HardwareTimers.hpp"
#include <chrono>
#include <functional>
#include <dynarray>
#include <Arduino.h>
namespace Timers_one_for_all
{
	// 所有可用的硬件计时器编号。此数组的索引即是软件号，值是硬件号。
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
	constexpr uint8_t _NumHardwareTimers =
#ifdef ARDUINO_ARCH_AVR
		6
#endif
#ifdef ARDUINO_ARCH_SAN
		9
#endif
		;
	// 可用的计时器个数。
	constexpr uint8_t NumTimers = std::extent<decltype(HardwareTimers)>::value;
	struct _TimerInterrupt
	{
		std::function<void()> Overflow;
		std::function<void()> CompareA;
		std::function<void()> CompareB;
	};
	extern _TimerInterrupt _TimerInterrupts[_NumHardwareTimers];
	// 计时器能够支持的最小的计时单位
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	// 指示每个软件计时器是否空闲的逻辑值。此数组用于自动调度，也可以辅助用户手动调度。
	extern bool TimerFree[NumTimers];
	// 返回值是计时器的软件号，范围[0,NumTimers)。使用HardwareTimers数组以将软件号映射为硬件号。如果没有空闲的计时器，返回NumTimers。
	uint8_t AllocateSoftwareTimer();
	// 将指定软件号的计时器标记为空闲，使其可以被再分配，但不会停止任何执行中的进程。输入已空闲的软件号不做任何事。输入大于等于NumTimers的无效软件号是未定义行为。
	inline void FreeSoftwareTimer(uint8_t SoftwareIndex) { TimerFree[SoftwareIndex] = true; }
	template <uint8_t Hardware, uint8_t Software = 0>
	struct _GetSoftware
	{
		static constexpr uint8_t value = Hardware == HardwareTimers[Software] ? Software : _GetSoftware<Hardware, Software + 1>::value;
	};
	template <uint8_t Hardware>
	struct _GetSoftware<Hardware, NumTimers>
	{
		static constexpr uint8_t value = NumTimers;
	};
	template <uint8_t Hardware = 0, uint8_t Software = 0>
	struct _MaxHardware
	{
		static constexpr uint8_t value = _MaxHardware<max(Hardware, HardwareTimers[Software]), Software + 1>::value;
	};
	template <uint8_t Hardware>
	struct _MaxHardware<Hardware, NumTimers>
	{
		static constexpr uint8_t value = Hardware;
	};
	template <typename T = std::make_integer_sequence<uint8_t, _MaxHardware<>::value>>
	struct _HardwareToSoftware;
	template <uint8_t... Hardware>
	struct _HardwareToSoftware<std::integer_sequence<uint8_t, Hardware...>>
	{
		static constexpr uint8_t value[sizeof...(Hardware)] = {_GetSoftware<Hardware>::value...};
	};
	// 释放指定的硬件计时器。会自动计算其软件号并设置TimerFree。
	inline void FreeHardwareTimer(uint8_t HardwareIndex) { FreeSoftwareTimer(_HardwareToSoftware<>::value[HardwareIndex]); }
#ifdef ARDUINO_ARCH_AVR
	struct _CommonPrescalers
	{
		// 使用左移运算应用预分频
		static constexpr uint16_t Prescalers[] = {1, 3, 6, 8, 10};
		static constexpr uint8_t NumPrescalers = std::extent_v<decltype(Prescalers)>;
	};
	template <typename TCounter>
	struct _AvrTimer
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
	// AVR架构的1、3、4、5号计时器的共用类型
	struct HardwareTimer1 : public _AvrTimer<uint16_t>, public _CommonPrescalers
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
	// AVR架构的0号计时器特殊类型
	struct HardwareTimer0 : public _AvrTimer<uint8_t>, public _CommonPrescalers
	{
	};
	template <>
	constexpr HardwareTimer0 HardwareTimer<0>{TCCR0A, TCCR0B, TIFR0, TIMSK0, 1 << WGM01, TCNT0, OCR0A, OCR0B};
#endif
#ifdef TOFA_USE_TIMER1
	template <>
	constexpr HardwareTimer1 HardwareTimer<1>{TCCR1A, TCCR1B, TIFR1, TIMSK1, 1 << WGM12, TCNT1, OCR1A, OCR1B};
#endif
#ifdef TOFA_USE_TIMER2
	// AVR架构的2号计时器特殊类型
	struct HardwareTimer2 : public _AvrTimer<uint8_t>
	{
		// 使用左移运算应用预分频
		static constexpr uint16_t Prescalers[] = {1, 3, 5, 6, 7, 8, 10};
		static constexpr uint8_t NumPrescalers = std::extent_v<decltype(Prescalers)>;
	};
	template <>
	constexpr HardwareTimer2 HardwareTimer<2>{TCCR2A, TCCR2B, TIFR2, TIMSK2, 1 << WGM21, TCNT2, OCR2A, OCR2B};
#endif
#ifdef TOFA_USE_TIMER3
	template <>
	constexpr HardwareTimer1 HardwareTimer<3>{TCCR3A, TCCR3B, TIFR3, TIMSK3, 1 << WGM32, TCNT3, OCR3A, OCR3B};
#endif
#ifdef TOFA_USE_TIMER4
	template <>
	constexpr HardwareTimer1 HardwareTimer<4>{TCCR4A, TCCR4B, TIFR4, TIMSK4, 1 << WGM42, TCNT4, OCR4A, OCR4B};
#endif
#ifdef TOFA_USE_TIMER5
	template <>
	constexpr HardwareTimer1 HardwareTimer<5>{TCCR5A, TCCR5B, TIFR5, TIMSK5, 1 << WGM52, TCNT5, OCR5A, OCR5B};
#endif
#endif
}