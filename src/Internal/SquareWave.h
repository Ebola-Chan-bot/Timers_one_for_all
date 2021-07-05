#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	namespace Internal
	{
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr>
		void SquareWaveToLow();
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr>
		void SquareWaveToHigh()
		{
			DoAfter<TimerCode, HighMilliseconds, SquareWaveToLow<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, InfiniteLr>>();
			EfficientDigitalWrite<PinCode, HIGH>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr>
		void SquareWaveToLow()
		{
			if (InfiniteLr || --LR<TimerCode>)
				DoAfter<TimerCode, LowMilliseconds, SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, InfiniteLr>>();
			EfficientDigitalWrite<PinCode, LOW>();
		}
		//这里要求RepeatTimes不能超过uint32_t上限的一半，故干脆直接限制为int32_t
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int32_t RepeatTimes = -1>
		void InternalSW()
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				break;
			case 1:
				EfficientDigitalWrite<PinCode, HIGH>();
				DoAfter<TimerCode, HighMilliseconds, EfficientDigitalWrite<PinCode, LOW>>();
				break;
			default:
				if (HighMilliseconds == LowMilliseconds)
				{
					constexpr TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
					EfficientDigitalWrite<PinCode, HIGH>();
					SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>, (RepeatTimes < 0) ? -1 : RepeatTimes * 2 - 1>();
				}
				else
				{
					constexpr uint32_t TM = TimerMax[TimerCode];
					constexpr uint32_t FullCycle = HighMilliseconds + LowMilliseconds;
					constexpr TimerSetting TryTS = GetTimerSetting(TimerCode, FullCycle);
					constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
					if (TryTS.TCNT < TM)
					{
						TIMSK<TimerCode> = 0;
						if (Timer02)
						{
							TCCRA<TimerCode> = 2;
							TCCRB<TimerCode> = TryTS.PrescalerBits;
						}
						else
						{
							TCCRA<TimerCode> = 0;
							TCCRB<TimerCode> = TryTS.PrescalerBits + 8;
						}
						SetOCRA<TimerCode>(TryTS.TCNT);
						SetOCRB<TimerCode>(TryTS.TCNT * HighMilliseconds / FullCycle);
						LR<TimerCode> = RepeatTimes;
						COMPA<TimerCode> = EfficientDigitalWrite<PinCode, HIGH>;
						COMPB<TimerCode> = []
						{
							EfficientDigitalWrite<PinCode, LOW>();
							if (RepeatTimes > 0 && !--LR<TimerCode>)
								TIMSK<TimerCode> = 0;
						};
						EfficientDigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIFR<TimerCode> = 255;
						TIMSK<TimerCode> = 6;
					}
					else
						SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, (RepeatTimes < 0)>();
				}
			}
		}
		template <uint8_t TimerCode>
		static volatile uint16_t HM;
		template <uint8_t TimerCode>
		static volatile uint16_t LM;
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr>
		void SquareWaveToLow();
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr>
		void SquareWaveToHigh()
		{
			DoAfter<TimerCode, SquareWaveToLow<TimerCode, PinCode, InfiniteLr>>(HM<TimerCode>);
			EfficientDigitalWrite<PinCode, HIGH>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr>
		void SquareWaveToLow()
		{
			if (InfiniteLr || --LR<TimerCode>)
				DoAfter<TimerCode, SquareWaveToHigh<TimerCode, PinCode, InfiniteLr>>(LM<TimerCode>);
			EfficientDigitalWrite<PinCode, LOW>();
		}
		//这里要求RepeatTimes不能超过uint32_t上限的一半，故干脆直接限制为int32_t
		template <uint8_t TimerCode, uint8_t PinCode, int32_t RepeatTimes = -1>
		void InternalSW(uint16_t HighMilliseconds, uint16_t LowMilliseconds)
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				break;
			case 1:
				EfficientDigitalWrite<PinCode, HIGH>();
				DoAfter<TimerCode, EfficientDigitalWrite<PinCode, LOW>>(HighMilliseconds);
				break;
			default:
				if (HighMilliseconds == LowMilliseconds)
				{
					TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
					EfficientDigitalWrite<PinCode, HIGH>();
					SLRepeaterSet<TimerCode, EfficientDigitalToggle<PinCode>, (RepeatTimes < 0) ? -1 : RepeatTimes * 2 - 1>(TS.TCNT, TS.PrescalerBits);
				}
				else
				{
					constexpr uint32_t TM = TimerMax[TimerCode];
					uint32_t FullCycle = HighMilliseconds + LowMilliseconds;
					TimerSetting TryTS = GetTimerSetting(TimerCode, FullCycle);
					constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
					if (TryTS.TCNT < TM)
					{
						TIMSK<TimerCode> = 0;
						if (Timer02)
						{
							TCCRA<TimerCode> = 2;
							TCCRB<TimerCode> = TryTS.PrescalerBits;
						}
						else
						{
							TCCRA<TimerCode> = 0;
							TCCRB<TimerCode> = TryTS.PrescalerBits + 8;
						}
						SetOCRA<TimerCode>(TryTS.TCNT);
						SetOCRB<TimerCode>(TryTS.TCNT * HighMilliseconds / FullCycle);
						LR<TimerCode> = RepeatTimes;
						COMPA<TimerCode> = EfficientDigitalWrite<PinCode, HIGH>;
						COMPB<TimerCode> = []
						{
							EfficientDigitalWrite<PinCode, LOW>();
							if (RepeatTimes > 0 && !--LR<TimerCode>)
								TIMSK<TimerCode> = 0;
						};
						EfficientDigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIFR<TimerCode> = 255;
						TIMSK<TimerCode> = 6;
					}
					else
					{
						HM<TimerCode> = HighMilliseconds;
						LM<TimerCode> = LowMilliseconds;
						SquareWaveToHigh<TimerCode, PinCode, (RepeatTimes < 0)>();
					}
				}
			}
		}
	}
	//生成高低电平时长不一的方波，无限循环
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
	void SquareWave()
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>();
	}
	//生成循环数有限的方波
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int16_t RepeatTimes>
	void SquareWave()
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, RepeatTimes>();
	}
	//生成高低电平时长不一的方波，无限循环
	template <uint8_t TimerCode, uint8_t PinCode>
	void SquareWave(uint16_t HighMilliseconds, uint16_t LowMilliseconds)
	{
		Internal::InternalSW<TimerCode, PinCode>(HighMilliseconds, LowMilliseconds);
	}
	//生成循环数有限的方波
	template <uint8_t TimerCode, uint8_t PinCode, int16_t RepeatTimes>
	void SquareWave(uint16_t HighMilliseconds, uint16_t LowMilliseconds)
	{
		Internal::InternalSW<TimerCode, PinCode, RepeatTimes>(HighMilliseconds, LowMilliseconds);
	}
}