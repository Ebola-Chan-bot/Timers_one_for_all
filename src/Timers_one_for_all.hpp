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
#include <stdint.h>
namespace Timers_one_for_all
{
	// API等级0：底层硬件细节，追求极致优化，0代价抽象，但使用复杂，需要对硬件有一定了解
	// API等级1：计时器分为初始化、设置中断
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
	
}