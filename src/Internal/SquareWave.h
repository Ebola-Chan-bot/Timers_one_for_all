#pragma once
#include "Kernel.h"
namespace TimersOneForAll
{
	namespace Internal
	{
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
		void SquareWaveToLow();
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
		void SquareWaveToHigh()
		{
			EfficientDigitalWrite<PinCode, HIGH>();
			DoAfter<TimerCode, HighMilliseconds, SquareWaveToLow<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
		void SquareWaveToLow()
		{
			EfficientDigitalWrite<PinCode, LOW>();
			DoAfter<TimerCode, LowMilliseconds, SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
		void InternalSW()
		{
			if (HighMilliseconds == LowMilliseconds)
			{
				constexpr TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
				EfficientDigitalWrite<PinCode, HIGH>();
				SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>>();
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
					COMPA<TimerCode> = EfficientDigitalWrite<PinCode, HIGH>;
					COMPB<TimerCode> = EfficientDigitalWrite<PinCode, LOW>;
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 6;
					EfficientDigitalWrite<PinCode, HIGH>();
				}
				else
				{
					constexpr bool HGL = HighMilliseconds > LowMilliseconds;
					constexpr uint16_t LongMilliseconds = HGL ? HighMilliseconds : LowMilliseconds;
					constexpr uint16_t ShortMilliseconds = HGL ? LowMilliseconds : HighMilliseconds;
					constexpr TimerSetting TS = GetTimerSetting(TimerCode, LongMilliseconds);
					if (TS.TCNT < TM)
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
						constexpr uint16_t ShortTCNT = uint32_t(TS.TCNT) * ShortMilliseconds / LongMilliseconds;
						constexpr uint16_t HighTCNT = HGL ? TS.TCNT : ShortTCNT;
						constexpr uint16_t LowTCNT = HGL ? ShortTCNT : TS.TCNT;
						SetOCRA<TimerCode>(HighTCNT);
						COMPA<TimerCode> = CompaSquareWave<TimerCode, PinCode, HighTCNT, LowTCNT>;
						EfficientDigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 2;
					}
					else
						SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>();
				}
			}
		}
		//这里要求RepeatTimes不能超过uint32_t上限的一半，故干脆直接限制为int32_t
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int32_t RepeatTimes>
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
					SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>, RepeatTimes * 2 - 1>();
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
							if (!--LR<TimerCode>)
								TIMSK<TimerCode> = 0;
						};
						EfficientDigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 6;
					}
					else
					{
						constexpr bool HGL = HighMilliseconds > LowMilliseconds;
						constexpr uint16_t LongMilliseconds = HGL ? HighMilliseconds : LowMilliseconds;
						constexpr uint16_t ShortMilliseconds = HGL ? LowMilliseconds : HighMilliseconds;
						constexpr TimerSetting TS = GetTimerSetting(TimerCode, LongMilliseconds);
						if (TS.TCNT < TM)
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
							constexpr uint16_t ShortTCNT = uint32_t(TS.TCNT) * ShortMilliseconds / LongMilliseconds;
							constexpr uint16_t HighTCNT = HGL ? TS.TCNT : ShortTCNT;
							constexpr uint16_t LowTCNT = HGL ? ShortTCNT : TS.TCNT;
							SetOCRA<TimerCode>(HighTCNT);
							LR<TimerCode> = RepeatTimes * 2 - 1;
							COMPA<TimerCode> = CompaSquareWaveT<TimerCode, PinCode, HighTCNT, LowTCNT>;
							EfficientDigitalWrite<PinCode, HIGH>();
							SetTCNT<TimerCode>(0);
							TIMSK<TimerCode> = 2;
						}
						else
							SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>();
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
}