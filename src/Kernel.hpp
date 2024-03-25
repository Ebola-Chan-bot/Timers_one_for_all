#pragma once
#include "HardwareTimers.hpp"
#include <chrono>
#include <functional>
#include <dynarray>
#include <utility>
#include <Arduino.h>
namespace Timers_one_for_all
{
	// 使用此数组将软件号索引到硬件号
	constexpr uint8_t SoftwareToHardware[] = {
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
#ifdef TOFA_USE_SYSTICK
		9,
#endif
	};
	// 可用的计时器个数。
	constexpr uint8_t NumTimers = std::extent<decltype(SoftwareToHardware)>::value;
	template <uint8_t Hardware, uint8_t Software = 0>
	struct _GetSoftware
	{
		static constexpr uint8_t value = Hardware == SoftwareToHardware[Software] ? Software : _GetSoftware<Hardware, Software + 1>::value;
	};
	template <uint8_t Hardware>
	struct _GetSoftware<Hardware, NumTimers>
	{
		static constexpr uint8_t value = NumTimers;
	};
	template <uint8_t Hardware = 0, uint8_t Software = 0>
	struct _MaxHardware
	{
		static constexpr uint8_t value = _MaxHardware<max(Hardware, SoftwareToHardware[Software]), Software + 1>::value;
	};
	template <uint8_t Hardware>
	struct _MaxHardware<Hardware, NumTimers>
	{
		static constexpr uint8_t value = Hardware;
	};
	template <typename T = std::make_integer_sequence<uint8_t, _MaxHardware<>::value + 1>>
	struct _HardwareToSoftware;
	template <uint8_t... Hardware>
	struct _HardwareToSoftware<std::integer_sequence<uint8_t, Hardware...>>
	{
		static constexpr uint8_t value[sizeof...(Hardware)] = {_GetSoftware<Hardware>::value...};
	};
	// 使用此数组将硬件号（有效的）索引到软件号。无效的硬件号将索引到NumTimers
	constexpr const uint8_t (&HardwareToSoftware)[_MaxHardware<>::value + 1] = _HardwareToSoftware<>::value;
	// 计时器能够支持的最小的计时单位
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	// 指示每个软件计时器是否空闲的逻辑值。此数组用于自动调度，也可以辅助用户手动调度。
	// 返回值是计时器的软件号，范围[0,NumTimers)。使用HardwareTimers数组以将软件号映射为硬件号。如果没有空闲的计时器，返回NumTimers。
	uint8_t AllocateSoftwareTimer();
	template <uint8_t SoftwareIndex>
	constexpr uint8_t _CheckSoftwareIndex()
	{
		static_assert(SoftwareIndex < NumTimers, "指定的计时器软件号超出范围");
		return SoftwareIndex;
	}
	template <uint8_t HardwareIndex>
	constexpr uint8_t _CheckHardwareIndex()
	{
		static_assert(HardwareIndex <= _MaxHardware<>::value && HardwareToSoftware[HardwareIndex] < NumTimers, "指定的硬件计时器不存在或未启用");
		return HardwareIndex;
	}
#ifdef ARDUINO_ARCH_AVR
	// 记录每个软件计时器的中断回调和空闲状态
	struct TimerState
	{
		std::function<void()> Overflow;
		std::function<void()> CompareA;
		std::function<void()> CompareB;
		bool TimerFree = true;
	};
	// 记录每个软件计时器的中断回调和空闲状态
	extern TimerState TimerStates[NumTimers];
	// 将指定软件号的计时器标记为空闲，使其可以被再分配，但不会停止任何执行中的进程。输入已空闲的软件号不做任何事。输入大于等于NumTimers的无效软件号是未定义行为。
	inline void FreeSoftwareTimer(uint8_t SoftwareIndex) { TimerStates[SoftwareIndex].TimerFree = true; }
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
	constexpr auto _HardwareTimer = []()
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
	constexpr HardwareTimer0 _HardwareTimer<0>{TCCR0A, TCCR0B, TIFR0, TIMSK0, 1 << WGM01, TCNT0, OCR0A, OCR0B};
#endif
#ifdef TOFA_USE_TIMER1
	template <>
	constexpr HardwareTimer1 _HardwareTimer<1>{TCCR1A, TCCR1B, TIFR1, TIMSK1, 1 << WGM12, TCNT1, OCR1A, OCR1B};
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
	constexpr HardwareTimer2 _HardwareTimer<2>{TCCR2A, TCCR2B, TIFR2, TIMSK2, 1 << WGM21, TCNT2, OCR2A, OCR2B};
#endif
#ifdef TOFA_USE_TIMER3
	template <>
	constexpr HardwareTimer1 _HardwareTimer<3>{TCCR3A, TCCR3B, TIFR3, TIMSK3, 1 << WGM32, TCNT3, OCR3A, OCR3B};
#endif
#ifdef TOFA_USE_TIMER4
	template <>
	constexpr HardwareTimer1 _HardwareTimer<4>{TCCR4A, TCCR4B, TIFR4, TIMSK4, 1 << WGM42, TCNT4, OCR4A, OCR4B};
#endif
#ifdef TOFA_USE_TIMER5
	template <>
	constexpr HardwareTimer1 _HardwareTimer<5>{TCCR5A, TCCR5B, TIFR5, TIMSK5, 1 << WGM52, TCNT5, OCR5A, OCR5B};
#endif
#endif
#ifdef ARDUINO_ARCH_SAM
	constexpr struct PeripheralTimer
	{
		TcChannel &Channel;
		IRQn_Type irq;
	} PeripheralTimers[] = {
#ifdef TOFA_USE_TIMER0
		{TC0->TC_CHANNEL[0], TC0_IRQn},
#endif
#ifdef TOFA_USE_TIMER1
		{TC0->TC_CHANNEL[1], TC1_IRQn},
#endif
#ifdef TOFA_USE_TIMER2
		{TC0->TC_CHANNEL[2], TC2_IRQn},
#endif
#ifdef TOFA_USE_TIMER3
		{TC1->TC_CHANNEL[0], TC3_IRQn},
#endif
#ifdef TOFA_USE_TIMER4
		{TC1->TC_CHANNEL[1], TC4_IRQn},
#endif
#ifdef TOFA_USE_TIMER5
		{TC1->TC_CHANNEL[2], TC5_IRQn},
#endif
#ifdef TOFA_USE_TIMER6
		{TC2->TC_CHANNEL[0], TC6_IRQn},
#endif
#ifdef TOFA_USE_TIMER7
		{TC2->TC_CHANNEL[1], TC7_IRQn},
#endif
#ifdef TOFA_USE_TIMER8
		{TC2->TC_CHANNEL[2], TC8_IRQn},
#endif
	};
	template <typename T = std::make_integer_sequence<uint8_t, NumTimers>>
	struct TimerState;
	template <uint8_t... SoftwareIndex>
	struct TimerState<std::integer_sequence<uint8_t, SoftwareIndex...>>
	{
		std::function<void()> InterruptHandler;
		bool TimerFree = true;
		TimerState(uint8_t SoftwareIndex);
		static TimerState<> TimerStates[NumTimers];
	};
	template <uint8_t... SoftwareIndex>
	TimerState<> TimerState<std::integer_sequence<uint8_t, SoftwareIndex...>>::TimerStates[NumTimers] = {SoftwareIndex...};
	// 将指定软件号的计时器标记为空闲，使其可以被再分配，但不会停止任何执行中的进程。输入已空闲的软件号不做任何事。输入大于等于NumTimers的无效软件号是未定义行为。
	inline void FreeSoftwareTimer(uint8_t SoftwareIndex) { TimerState<>::TimerStates[SoftwareIndex].TimerFree = true; }
#endif
}