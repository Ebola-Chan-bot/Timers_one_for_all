#pragma once
#include "Kernel.h"
#include <LowLevelQuickDigitalIO.h>
using namespace LowLevelQuickDigitalIO;
namespace TimersOneForAll
{
	// 无限播放音调
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		DigitalToggle<PinCode>();
		Internal::TimerTask<TimerCode> = DigitalToggle<PinCode>;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, -1, nullptr>();
	}
	// 播放有限的毫秒数
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds, void (*DoneCallback)() = nullptr>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		DigitalToggle<PinCode>();
		Internal::TimerTask<TimerCode> = DigitalToggle<PinCode>;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, uint32_t(FrequencyHz) * Milliseconds / 500, DoneCallback>();
	}
	// 播放有限的毫秒数
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, void (*DoneCallback)() = nullptr>
	void PlayTone(uint16_t Milliseconds)
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		DigitalToggle<PinCode>();
		Internal::TimerTask<TimerCode> = DigitalToggle<PinCode>;
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, DoneCallback>(uint32_t(FrequencyHz) * Milliseconds / 500);
	}
}