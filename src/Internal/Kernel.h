#pragma once
#include <Arduino.h>
namespace TimersOneForAll
{
	//取得上次调用StartTiming以来经过的毫秒数
	template <uint8_t TimerCode>
	static volatile uint16_t MillisecondsElapsed;
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
#pragma region 内部功能实现核心
		template <uint8_t TimerCode>
		volatile static uint32_t SR;
		template <uint8_t TimerCode>
		volatile static int32_t LR;
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
		//Compa0表示自我重复，不切换
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t SmallRepeats, bool InfiniteLr>
		void Compa0()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
					SR<TimerCode> = SmallRepeats;
				else
					TIMSK<TimerCode> = 0;
				DoTask();
			}
		}
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr>
		void Compa2();
		//Compa1是小重复，结束后要切换到大重复，但是大重复有可能是COMPA也可能是OVF
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr>
		void Compa1()
		{
			if (!--SR<TimerCode>)
			{
				if (Tcnt2 < TimerMax[TimerCode])
				{
					SetOCRA<TimerCode>(Tcnt2);
					COMPA<TimerCode> = Compa2<TimerCode, DoTask, Tcnt1, Tcnt2, SR1, SR2, InfiniteLr>;
				}
				else
					TIMSK<TimerCode> = 1;
				SR<TimerCode> = SR2;
			}
		}
		//Compa2是大重复，结束后要执行动作，如果有重复次数还要切换到小重复
		template <uint8_t TimerCode, void (*DoTask)(), uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr>
		void Compa2()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
				{
					if (Tcnt2 < TimerMax[TimerCode])
					{
						SetOCRA<TimerCode>(Tcnt1);
						COMPA<TimerCode> = Compa1<TimerCode, DoTask, Tcnt1, Tcnt2, SR1, SR2, InfiniteLr>;
					}
					else
						TIMSK<TimerCode> = 2;
					SR<TimerCode> = SR1;
				}
				else
					TIMSK<TimerCode> = 0;
				DoTask();
			}
		}
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoTask)(), int32_t RepeatTimes = -1>
		void SLRepeaterSet()
		{
			TIMSK<TimerCode> = 0;
			if (RepeatTimes != 0)
			{
				constexpr uint32_t TM = TimerMax[TimerCode];
				constexpr uint16_t IdealRepeats = TCNT / (TM - (TimerCode == 0));
				constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
				if (Timer02)
					TCCRB<TimerCode> = PrescalerBits;
				else
					TCCRA<TimerCode> = 0;
				if (RepeatTimes > 0)
					LR<TimerCode> = RepeatTimes;
				if (IdealRepeats * TM < TCNT || TimerCode == 0) //Timer0的OVF不可用
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					constexpr uint16_t ActualRepeats = IdealRepeats + 1;
					constexpr uint16_t Tcnt1 = TCNT / ActualRepeats;
					constexpr uint16_t Tcnt2 = Tcnt1 + 1;
					constexpr uint16_t SR1 = ActualRepeats * Tcnt2 - TCNT; //必大于0
					constexpr uint16_t SR2 = TCNT - ActualRepeats * Tcnt1;
					SetOCRA<TimerCode>(Tcnt1);
					SR<TimerCode> = SR1;
					if (SR2)
					{
						COMPA<TimerCode> = Compa1<TimerCode, DoTask, Tcnt1, Tcnt2, SR1, SR2, (RepeatTimes < 0)>;
						if (Tcnt2 == TM)
							OVF<TimerCode> = Compa2<TimerCode, DoTask, Tcnt1, Tcnt2, SR1, SR2, (RepeatTimes < 0)>;
					}
					else
						COMPA<TimerCode> = Compa0<TimerCode, DoTask, SR1, (RepeatTimes < 0)>;
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 2;
				}
				else
				{
					SR<TimerCode> = IdealRepeats;
					OVF<TimerCode> = Compa0<TimerCode, DoTask, IdealRepeats, (RepeatTimes < 0)>;
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetTCNT<TimerCode>(0);
					TIMSK<TimerCode> = 1;
				}
			}
		}
	}
}
