#pragma once
#ifndef ARDUINO_ARCH_AVR
#error 仅支持AVR架构
#endif
#include <Arduino.h>
namespace TimersOneForAll
{
	template <uint8_t TimerCode>
	static uint16_t MillisecondsElapsed;
	template <uint8_t TimerCode, uint16_t AfterMilliseconds, void (*DoTask)()>
	void DoAfter();
	namespace Internal
	{
#pragma region 预分频器
		struct TimerSetting
		{
			uint8_t PrescalerBits;
			uint32_t TCNT;
		};
		constexpr uint32_t TimerMax[] = {256, 65536, 256, 65536, 65536, 65536};
		constexpr TimerSetting SelectPrescaler(uint8_t TimerCode, float Milliseconds, const uint16_t ValidPrescalers[], uint8_t NoVPs)
		{
			float NoOscillations = F_CPU * Milliseconds / 1000;
			float IdealPrescaler = NoOscillations / (TimerMax[TimerCode] - (TimerCode == 0));
			uint8_t PrescalerBits = 0;
			uint16_t ActualPrescaler = 0;
			for (; (ActualPrescaler = ValidPrescalers[PrescalerBits++]) < IdealPrescaler && PrescalerBits < NoVPs;)
				;
			return {PrescalerBits, NoOscillations / ActualPrescaler};
		};
		constexpr TimerSetting GetTimerSetting(uint8_t TimerCode, float Miliseconds)
		{
			constexpr uint16_t ValidPrescalers[] = {1, 8, 64, 256, 1024};
			constexpr uint16_t ValidPrescalers2[] = {1, 8, 32, 64, 128, 256, 1024};
			bool TC2 = TimerCode == 2;
			return SelectPrescaler(TimerCode, Miliseconds, TC2 ? ValidPrescalers2 : ValidPrescalers, TC2 ? 7 : 5);
		};
#pragma endregion
#pragma region 硬件寄存器
		template <uint8_t TimerCode>
		volatile uint8_t &TCCRA;
		template <uint8_t TimerCode>
		volatile uint8_t &TCCRB;
		//TCNT有可能是uint8_t或uint16_t类型，因此不能直接取引用
		template <uint8_t TimerCode>
		uint16_t GetTCNT();
		template <uint8_t TimerCode>
		void SetTCNT(uint16_t TCNT);
		template <uint8_t TimerCode>
		volatile uint8_t &TIMSK;
		template <uint8_t TimerCode>
		uint16_t GetOCRA();
		template <uint8_t TimerCode>
		void SetOCRA(uint16_t OCRA);
		template <uint8_t TimerCode>
		uint16_t GetOCRB();
		template <uint8_t TimerCode>
		void SetOCRB(uint16_t OCRB);
		template <uint8_t TimerCode>
		void (*volatile OVF)();
		template <uint8_t TimerCode>
		void (*volatile COMPA)();
		template <uint8_t TimerCode>
		void (*volatile COMPB)();
#define TimerSpecialize(Code)                      \
	template <>                                    \
	volatile uint8_t &TCCRA<Code> = TCCR##Code##A; \
	template <>                                    \
	volatile uint8_t &TCCRB<Code> = TCCR##Code##B; \
	template <>                                    \
	volatile uint8_t &TIMSK<Code> = TIMSK##Code;   \
	template <>                                    \
	uint16_t GetTCNT<Code>()                       \
	{                                              \
		return TCNT##Code;                         \
	}                                              \
	template <>                                    \
	void SetTCNT<Code>(uint16_t TCNT)              \
	{                                              \
		TCNT##Code = TCNT;                         \
	}                                              \
	template <>                                    \
	uint16_t GetOCRA<Code>()                       \
	{                                              \
		return OCR##Code##A;                       \
	}                                              \
	template <>                                    \
	void SetOCRA<Code>(uint16_t OCRA)              \
	{                                              \
		OCR##Code##A = OCRA;                       \
	}                                              \
	template <>                                    \
	uint16_t GetOCRB<Code>()                       \
	{                                              \
		return OCR##Code##B;                       \
	}                                              \
	template <>                                    \
	void SetOCRB<Code>(uint16_t OCRB)              \
	{                                              \
		OCR##Code##B = OCRB;                       \
	}                                              \
	ISR(TIMER##Code##_COMPA_vect)                  \
	{                                              \
		COMPA<Code>();                             \
	}                                              \
	ISR(TIMER##Code##_COMPB_vect)                  \
	{                                              \
		COMPB<Code>();                             \
	}
#ifdef TCNT0
		TimerSpecialize(0);
//Timer0的溢出中断被内置millis()函数占用了，无法使用
#endif
#ifdef TCNT1
		TimerSpecialize(1);
		ISR(TIMER1_OVF_vect)
		{
			OVF<1>();
		}
#endif
#ifdef TCNT2
		TimerSpecialize(2);
		ISR(TIMER2_OVF_vect)
		{
			OVF<2>();
		}
#endif
#ifdef TCNT3
		TimerSpecialize(3);
		ISR(TIMER3_OVF_vect)
		{
			OVF<3>();
		}
#endif
#ifdef TCNT4
		TimerSpecialize(4);
		ISR(TIMER4_OVF_vect)
		{
			OVF<4>();
		}
#endif
#ifdef TCNT5
		TimerSpecialize(5);
		ISR(TIMER5_OVF_vect)
		{
			OVF<5>();
		}
#endif
#pragma endregion
#pragma region 全局状态参量
		template <uint8_t TimerCode>
		volatile static uint32_t SR;
		template <uint8_t TimerCode>
		volatile static uint32_t LR;
#pragma endregion
#pragma region 内部功能实现核心
		template <uint8_t TimerCode, void (*DoTask)()>
		void CompaDLarge()
		{
			if (!--LR<TimerCode>)
			{
				TIMSK<TimerCode> = 0;
				DoTask();
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t TCNTLarge>
		void CompaDSmall()
		{
			if (!--SR<TimerCode>)
			{
				SetOCRA<TimerCode>(TCNTLarge);
				COMPA<TimerCode> = CompaDLarge<TimerCode, DoTask>;
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRLarge();
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRSmall()
		{
			if (!--SR<TimerCode>)
			{
				SR<TimerCode> = LargeRepeats;
				if (TCNTLarge < TimerMax[TimerCode])
				{
					SetOCRA<TimerCode>(TCNTLarge);
					COMPA<TimerCode> = CompaRLarge<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
				}
				else
					TIMSK<TimerCode> = 1;
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRLarge()
		{
			if (!--SR<TimerCode>)
			{
				SR<TimerCode> = SmallRepeats;
				if (TCNTLarge < TimerMax[TimerCode])
				{
					SetOCRA<TimerCode>(TCNTSmall);
					COMPA<TimerCode> = CompaRSmall<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
				}
				else
					TIMSK<TimerCode> = 2;
				DoTask();
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t SelfRepeats>
		void CompaRSelf()
		{
			if (!--SR<TimerCode>)
			{
				SR<TimerCode> = SelfRepeats;
				DoTask();
			}
		}
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRLargeT();
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRSmallT()
		{
			if (!--SR<TimerCode>)
			{
				SR<TimerCode> = LargeRepeats;
				if (TCNTLarge < TimerMax[TimerCode])
				{
					SetOCRA<TimerCode>(TCNTLarge);
					COMPA<TimerCode> = CompaRLargeT<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
				}
				else
					TIMSK<TimerCode> = 1;
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t LargeRepeats, uint16_t TCNTLarge, uint16_t SmallRepeats, uint16_t TCNTSmall>
		void CompaRLargeT()
		{
			if (!--SR<TimerCode>)
			{
				if (--LR<TimerCode>)
				{
					SR<TimerCode> = SmallRepeats;
					if (TCNTLarge < TimerMax[TimerCode])
					{
						SetOCRA<TimerCode>(TCNTSmall);
						COMPA<TimerCode> = CompaRSmallT<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
					}
					else
						TIMSK<TimerCode> = 2;
				}
				else
					TIMSK<TimerCode> = 0;
				DoTask();
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t SelfRepeats>
		void CompaRSelfRepeat()
		{
			if (!--SR<TimerCode>)
			{
				if (--LR<TimerCode>)
					SR<TimerCode> = SelfRepeats;
				else
					TIMSK<TimerCode> = 0;
				DoTask();
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint32_t IdealRepeats>
		void OvfR()
		{
			if (!--SR<TimerCode>)
			{
				SR<TimerCode> = IdealRepeats;
				DoTask();
			}
		};
		template <uint8_t TimerCode, void (*DoTask)(), uint32_t IdealRepeats>
		void OvfRT()
		{
			if (!--SR<TimerCode>)
			{
				if (--LR<TimerCode>)
					SR<TimerCode> = IdealRepeats;
				else
					TIMSK<TimerCode> = 0;
				DoTask();
			}
		};
		template <uint8_t PinCode>
		void EfficientDigitalToggle()
		{
			volatile uint8_t *const PinPort = portOutputRegister(digitalPinToPort(PinCode));
			const uint8_t BitMask = digitalPinToBitMask(PinCode);
			*PinPort ^= BitMask;
		}
		template <uint8_t PinCode, bool IsHigh>
		void EfficientDigitalWrite()
		{
			volatile uint8_t *const PinPort = portOutputRegister(digitalPinToPort(PinCode));
			const uint8_t BitMask = digitalPinToBitMask(PinCode);
			if (IsHigh)
				*PinPort |= BitMask;
			else
				*PinPort &= ~BitMask;
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighTCNT, uint16_t LowTCNT>
		void CompaSquareWave()
		{
			SetOCRA<TimerCode>(GetOCRA<TimerCode>() == HighTCNT ? LowTCNT : HighTCNT);
			EfficientDigitalToggle<PinCode>();
		}
		template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighTCNT, uint16_t LowTCNT>
		void CompaSquareWaveT()
		{
			SetOCRA<TimerCode>(GetOCRA<TimerCode>() == HighTCNT ? LowTCNT : HighTCNT);
			EfficientDigitalToggle<PinCode>();
			if (!--LR<TimerCode>)
				TIMSK<TimerCode> = 0;
		}
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoTask)()>
		void InternalDA()
		{
			constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
			if (TCNT)
			{
				constexpr uint32_t TM = TimerMax[TimerCode];
				TIMSK<TimerCode> = 0;
				if (Timer02)
					TCCRB<TimerCode> = PrescalerBits;
				else
					TCCRA<TimerCode> = 0;
				//拒绝外部振荡源
				bitWrite(ASSR, AS2, false);
				if (TCNT < TM)
				{
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetOCRA<TimerCode>(TCNT);
					COMPA<TimerCode> = []
					{
						TIMSK<TimerCode> = 0;
						DoTask();
					};
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 2;
				}
				else
				{
					constexpr uint16_t IdealRepeats = TCNT / (TM - (TimerCode == 0));
					if (IdealRepeats * TM < TCNT || TimerCode == 0) //Timer0的OVF不可用
					{
						constexpr uint16_t ActualRepeats = IdealRepeats + 1;
						constexpr uint16_t TCNTSmall = TCNT / ActualRepeats;
						constexpr uint16_t TCNTLarge = TCNTSmall + 1;
						constexpr uint16_t SmallRepeats = ActualRepeats * TCNTLarge - TCNT;
						constexpr uint16_t LargeRepeats = TCNT - ActualRepeats * TCNTSmall;
						if (Timer02)
							TCCRA<TimerCode> = 2;
						else
							TCCRB<TimerCode> = PrescalerBits + 8;
						if (SmallRepeats)
						{
							SetOCRA<TimerCode>(TCNTSmall);
							if (LargeRepeats)
							{
								SR<TimerCode> = SmallRepeats;
								LR<TimerCode> = LargeRepeats;
								COMPA<TimerCode> = CompaDSmall<TimerCode, DoTask, TCNTLarge>;
							}
							else
							{
								//大重复为0，小重复晋级为大重复
								LR<TimerCode> = SmallRepeats;
								COMPA<TimerCode> = CompaDLarge<TimerCode, DoTask>;
							}
						}
						else
						{
							//小重复为0，大重复必不为0，直接开始
							SetOCRA<TimerCode>(TCNTLarge);
							LR<TimerCode> = LargeRepeats;
							COMPA<TimerCode> = CompaDLarge<TimerCode, DoTask>;
						}
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 2;
					}
					else
					{
						SR<TimerCode> = IdealRepeats;
						OVF<TimerCode> = []
						{
							if (!--SR<TimerCode>)
							{
								TIMSK<TimerCode> = 0;
								DoTask();
							}
						};
						if (Timer02)
							TCCRA<TimerCode> = 0;
						else
							TCCRB<TimerCode> = PrescalerBits;
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 1;
					}
				}
			}
			else
				DoTask();
		}
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoTask)()>
		void InternalRA()
		{
			constexpr uint32_t TM = TimerMax[TimerCode];
			TIMSK<TimerCode> = 0;
			constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
			if (Timer02)
				TCCRB<TimerCode> = PrescalerBits;
			else
				TCCRA<TimerCode> = 0;
			//拒绝外部振荡源
			bitWrite(ASSR, AS2, false);
			if (TCNT < TM)
			{
				if (Timer02)
					TCCRA<TimerCode> = 2;
				else
					TCCRB<TimerCode> = PrescalerBits + 8;
				SetOCRA<TimerCode>(TCNT);
				COMPA<TimerCode> = DoTask;
				SetTCNT<TimerCode>(0);
				TIMSK<TimerCode> = 2;
			}
			else
			{
				constexpr uint16_t IdealRepeats = TCNT / (TM - (TimerCode == 0)); //Timer0的OVF不可用，因此只能用COMPA，最大255
				if (IdealRepeats * TM < TCNT || TimerCode == 0)					  //Timer0的OVF不可用
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					constexpr uint16_t ActualRepeats = IdealRepeats + 1;
					constexpr uint16_t TCNTSmall = TCNT / ActualRepeats;
					constexpr uint16_t TCNTLarge = TCNTSmall + 1;
					constexpr uint16_t SmallRepeats = ActualRepeats * TCNTLarge - TCNT;
					constexpr uint16_t LargeRepeats = TCNT - ActualRepeats * TCNTSmall;
					if (SmallRepeats)
					{
						SR<TimerCode> = SmallRepeats;
						SetOCRA<TimerCode>(TCNTSmall);
						if (LargeRepeats)
						{
							COMPA<TimerCode> = CompaRSmall<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
							if (TCNTLarge == TM)
								OVF<TimerCode> = CompaRLarge<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
						}
						else
							COMPA<TimerCode> = CompaRSelf<TimerCode, DoTask, SmallRepeats>;
					}
					else
					{
						SR<TimerCode> = LargeRepeats;
						if (TCNTLarge < TM)
						{
							SetOCRA<TimerCode>(TCNTLarge);
							COMPA<TimerCode> = CompaRSelf<TimerCode, DoTask, LargeRepeats>;
						}
						else
							OVF<TimerCode> = CompaRSelf<TimerCode, DoTask, LargeRepeats>;
					}
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 2;
				}
				else
				{
					SR<TimerCode> = IdealRepeats;
					OVF<TimerCode> = OvfR<TimerCode, DoTask, IdealRepeats>;
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 1;
				}
			}
		}
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoTask)(), uint32_t RepeatTimes>
		void InternalRA()
		{
			switch (RepeatTimes)
			{
			case 0:
				TIMSK<TimerCode> = 0;
				break;
			case 1:
				InternalDA<TimerCode, TCNT, PrescalerBits, DoTask>();
				break;
			default:
				constexpr uint32_t TM = TimerMax[TimerCode];
				TIMSK<TimerCode> = 0;
				constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
				if (Timer02)
					TCCRB<TimerCode> = PrescalerBits;
				else
					TCCRA<TimerCode> = 0;
				//拒绝外部振荡源
				bitWrite(ASSR, AS2, false);
				LR<TimerCode> = RepeatTimes;
				if (TCNT < TM)
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					SetOCRA<TimerCode>(TCNT);
					COMPA<TimerCode> = []
					{
						if (!--LR<TimerCode>)
							TIMSK<TimerCode> = 0;
						DoTask();
					};
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 2;
				}
				else
				{
					constexpr uint16_t IdealRepeats = TCNT / (TM - (TimerCode == 0));
					if (IdealRepeats * TM < TCNT || TimerCode == 0) //Timer0的OVF不可用
					{
						constexpr uint16_t ActualRepeats = IdealRepeats + 1;
						constexpr uint16_t TCNTSmall = TCNT / ActualRepeats;
						constexpr uint16_t TCNTLarge = TCNTSmall + 1;
						constexpr uint16_t SmallRepeats = ActualRepeats * TCNTLarge - TCNT;
						constexpr uint16_t LargeRepeats = TCNT - ActualRepeats * TCNTSmall;
						if (Timer02)
							TCCRA<TimerCode> = 2;
						else
							TCCRB<TimerCode> = PrescalerBits + 8;
						if (SmallRepeats)
						{
							SR<TimerCode> = SmallRepeats;
							SetOCRA<TimerCode>(TCNTSmall);
							if (LargeRepeats)
							{
								COMPA<TimerCode> = CompaRSmallT<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
								if (TCNTLarge == TM)
									OVF<TimerCode> = CompaRLargeT<TimerCode, DoTask, LargeRepeats, TCNTLarge, SmallRepeats, TCNTSmall>;
							}
							else
								COMPA<TimerCode> = CompaRSelfRepeat<TimerCode, DoTask, SmallRepeats>;
						}
						else
						{
							SR<TimerCode> = LargeRepeats;
							SetOCRA<TimerCode>(TCNTLarge);
							COMPA<TimerCode> = CompaRSelfRepeat<TimerCode, DoTask, LargeRepeats>;
						}
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 2;
					}
					else
					{
						SR<TimerCode> = IdealRepeats;
						OVF<TimerCode> = OvfRT<TimerCode, DoTask, IdealRepeats>;
						SetTCNT<TimerCode>(0);
						TIMSK<TimerCode> = 1;
					}
				}
			}
		}
		template <uint8_t TimerCode>
		void MEAdd()
		{
			MillisecondsElapsed<TimerCode> ++;
		}
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
				InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>>();
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
					InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>, RepeatTimes * 2 - 1>();
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
#pragma endregion
#pragma region 公开API
	template <uint8_t TimerCode, uint16_t AfterMilliseconds, void (*DoTask)()>
	void DoAfter()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, AfterMilliseconds);
		Internal::InternalDA<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask>();
	}
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)()>
	void RepeatAfter()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask>();
	}
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), uint32_t RepeatTimes>
	void RepeatAfter()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, IntervalMilliseconds);
		Internal::InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask, RepeatTimes>();
	}
	template <uint8_t TimerCode>
	void StartTiming()
	{
		MillisecondsElapsed<TimerCode> = 0;
		RepeatAfter<TimerCode, 1, Internal::MEAdd<TimerCode>>();
	}
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		Internal::EfficientDigitalToggle<PinCode>();
		Internal::InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, Internal::EfficientDigitalToggle<PinCode>>();
	}
	//在指定引脚上生成指定频率的方波，持续指定的毫秒数
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds>
	void PlayTone()
	{
		constexpr Internal::TimerSetting TS = Internal::GetTimerSetting(TimerCode, 500.f / FrequencyHz);
		Internal::EfficientDigitalToggle<PinCode>();
		Internal::InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, Internal::EfficientDigitalToggle<PinCode>, uint32_t(FrequencyHz) * Milliseconds / 500>();
	}
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
	void SquareWave()
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>();
	}
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int16_t RepeatTimes>
	void SquareWave()
	{
		Internal::InternalSW<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, RepeatTimes>();
	}
	template <uint8_t TimerCode>
	void ShutDown()
	{
		Internal::TIMSK<TimerCode> = 0;
	}
#pragma endregion
};