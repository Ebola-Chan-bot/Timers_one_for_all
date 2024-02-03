#pragma once
#include "Kernel.h"
#include <Low_level_quick_digital_IO.hpp>
namespace Timers_one_for_all
{
	namespace Internal
	{
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToLow();
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToHigh()
		{
			DoAfter<TimerCode, HighMilliseconds>(SquareWaveToLow<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, InfiniteLr, DoneCallback>);
			Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToLow()
		{
			Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
			if (InfiniteLr || --LR<TimerCode>)
				DoAfter<TimerCode, LowMilliseconds>(SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, InfiniteLr, DoneCallback>);
			else if (DoneCallback)
				DoneCallback();
		}
		template <uint8_t PinCode, void (*DoneCallback)()>
		void OneShotSWDone()
		{
			Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
			if (DoneCallback)
				DoneCallback();
		}
		// 这里要求RepeatTimes不能超过uint32_t上限的一半，故干脆直接限制为int32_t
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int32_t RepeatTimes, void (*DoneCallback)()>
		void InternalSW()
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				if (DoneCallback)
					DoneCallback();
				break;
			case 1:
				Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
				DoAfter<TimerCode, HighMilliseconds>(OneShotSWDone<PinCode, DoneCallback>);
				break;
			default:
				if (HighMilliseconds == LowMilliseconds)
				{
					constexpr TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
					Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
					TimerTask<TimerCode> = Low_level_quick_digital_IO::DigitalToggle<PinCode>;
					SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, (RepeatTimes < 0) ? -1 : RepeatTimes * 2 - 1, DoneCallback>();
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
						COMPA<TimerCode> = Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>;
						COMPB<TimerCode> = []
						{
							Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
							if (RepeatTimes > 0 && !--LR<TimerCode>)
							{
								TIMSK<TimerCode> = 0;
								if (DoneCallback)
									DoneCallback();
							}
						};
						Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIFR<TimerCode> = 255;
						TIMSK<TimerCode> = 6;
					}
					else
						SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, (RepeatTimes < 0), DoneCallback>();
				}
			}
		}
		template <uint8_t TimerCode>
		static volatile uint16_t HM;
		template <uint8_t TimerCode>
		static volatile uint16_t LM;
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToLow();
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToHigh()
		{
			DoAfter<TimerCode>(HM<TimerCode>, SquareWaveToLow<TimerCode, PinCode, InfiniteLr, DoneCallback>);
			Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, bool InfiniteLr, void (*DoneCallback)()>
		void SquareWaveToLow()
		{
			Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
			if (InfiniteLr || --LR<TimerCode>)
				DoAfter<TimerCode>(LM<TimerCode>, SquareWaveToHigh<TimerCode, PinCode, InfiniteLr, DoneCallback>);
			else if (DoneCallback)
				DoneCallback();
		}
		// 这里要求RepeatTimes不能超过uint32_t上限的一半，故干脆直接限制为int32_t
		template <uint8_t TimerCode, uint8_t PinCode, int32_t RepeatTimes, void (*DoneCallback)()>
		void InternalSW(uint16_t HighMilliseconds, uint16_t LowMilliseconds)
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				if (DoneCallback)
					DoneCallback();
				break;
			case 1:
				Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
				DoAfter<TimerCode>(HighMilliseconds, OneShotSWDone<PinCode, DoneCallback>);
				break;
			default:
				if (HighMilliseconds == LowMilliseconds)
				{
					TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
					Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
					TimerTask<TimerCode> = Low_level_quick_digital_IO::DigitalToggle<PinCode>;
					SLRepeaterSet<TimerCode, (RepeatTimes < 0) ? -1 : RepeatTimes * 2 - 1, DoneCallback>(TS.TCNT, TS.PrescalerBits);
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
						COMPA<TimerCode> = Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>;
						COMPB<TimerCode> = []
						{
							Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
							if (RepeatTimes > 0 && !--LR<TimerCode>)
							{
								TIMSK<TimerCode> = 0;
								if (DoneCallback)
									DoneCallback();
							}
						};
						Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
						SetTCNT<TimerCode>(0);
						TIFR<TimerCode> = 255;
						TIMSK<TimerCode> = 6;
					}
					else
					{
						HM<TimerCode> = HighMilliseconds;
						LM<TimerCode> = LowMilliseconds;
						SquareWaveToHigh<TimerCode, PinCode, (RepeatTimes < 0), DoneCallback>();
					}
				}
			}
		}
		// 重复次数可变实现
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, void (*DoneCallback)()>
		void InternalSW(int32_t RepeatTimes)
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				if (DoneCallback)
					DoneCallback();
				break;
			case 1:
				Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
				DoAfter<TimerCode, HighMilliseconds>(OneShotSWDone<PinCode, DoneCallback>);
				break;
			default:
				if (HighMilliseconds == LowMilliseconds)
				{
					constexpr TimerSetting TS = GetTimerSetting(TimerCode, HighMilliseconds);
					Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
					TimerTask<TimerCode> = Low_level_quick_digital_IO::DigitalToggle<PinCode>;
					SLRepeaterSet<TimerCode, TS.TCNT, TS.PrescalerBits, DoneCallback>(RepeatTimes < 0 ? -1 : RepeatTimes * 2 - 1);
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
						COMPA<TimerCode> = Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>;
						COMPB<TimerCode> = RepeatTimes > 0 ? (void (*)())[]
						{
							Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>();
							if (!--LR<TimerCode>)
							{
								TIMSK<TimerCode> = 0;
								if (DoneCallback)
									DoneCallback();
							}
						}
														   : Low_level_quick_digital_IO::DigitalWrite<PinCode, LOW>;
														   Low_level_quick_digital_IO::DigitalWrite<PinCode, HIGH>();
														   SetTCNT<TimerCode>(0);
														   TIFR<TimerCode> = 255;
														   TIMSK<TimerCode> = 6;
					}
					else if (RepeatTimes < 0)
														   SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, true, DoneCallback>();
					else
														   SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, false, DoneCallback>();
				}
			}
		}
	}
	// 生成循环数有限的方波。如不指定循环次数，默认无限循环
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int16_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void SquareWave()
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, RepeatTimes, DoneCallback>();
	}
	// 生成循环数有限的方波。如不指定循环次数，默认无限循环
	template <uint8_t TimerCode, uint8_t PinCode, int16_t RepeatTimes = -1, void (*DoneCallback)() = nullptr>
	void SquareWave(uint16_t HighMilliseconds, uint16_t LowMilliseconds)
	{
		Internal::InternalSW<TimerCode, PinCode, RepeatTimes, DoneCallback>(HighMilliseconds, LowMilliseconds);
	}
	// 生成循环数有限的方波。如不指定循环次数，默认无限循环
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, void (*DoneCallback)() = nullptr>
	void SquareWave(int16_t RepeatTimes)
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, DoneCallback>(RepeatTimes);
	}
}