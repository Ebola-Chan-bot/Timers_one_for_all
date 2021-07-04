#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{	
	//无限播放音调
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		Internal::EfficientDigitalToggle<PinCode>();
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, Internal::EfficientDigitalToggle<PinCode>>();
	}
	//播放有限的毫秒数
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		Internal::EfficientDigitalToggle<PinCode>();
		Internal::SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, Internal::EfficientDigitalToggle<PinCode>, uint32_t(FrequencyHz) * Milliseconds / 500>();
	}
}