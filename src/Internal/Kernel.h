#pragma once
#include <Arduino.h>
// #include "Debugger.h"
namespace TimersOneForAll
{
	// 不要使用这个命名空间，除非你很清楚自己在做什么
	namespace Internal
	{
#pragma region 预分频器
		struct TimerSetting
		{
			uint8_t PrescalerBits;
			uint32_t TCNT;
		};
		constexpr uint32_t TimerMax[] = {256, 65536, 256, 65536, 65536, 65536};
		constexpr uint8_t NoPrescalers[] = {5, 5, 7, 5, 5, 5};
		constexpr uint16_t ValidPrescalers[][7] = {
			{1, 8, 64, 256, 1024},
			{1, 8, 64, 256, 1024},
			{1, 8, 32, 64, 128, 256, 1024},
			{1, 8, 64, 256, 1024},
			{1, 8, 64, 256, 1024},
			{1, 8, 64, 256, 1024},
		};
#define TVNN(Code, Prescaler) TimerMax[Code] * ValidPrescalers[Code][Prescaler] / float(F_CPU) * 1000
#define TVNN5(Code) TVNN(Code, 0), TVNN(Code, 1), TVNN(Code, 2), TVNN(Code, 3), TVNN(Code, 4)
		constexpr float MaxMilliseconds[][7] = {
			{TVNN5(0)},
			{TVNN5(1)},
			{TVNN5(2), TVNN(2, 5), TVNN(2, 6)},
			{TVNN5(3)},
			{TVNN5(4)},
			{TVNN5(5)},
		};
		constexpr TimerSetting GetTimerSetting(uint8_t TimerCode, float Milliseconds)
		{
			uint8_t NPTC = NoPrescalers[TimerCode];
			uint8_t PrescalerBits = 0;
			for (; PrescalerBits < NPTC; ++PrescalerBits)
				if (Milliseconds < MaxMilliseconds[TimerCode][PrescalerBits])
					break;
			if (PrescalerBits == NPTC)
				return {PrescalerBits, F_CPU * Milliseconds / 1000 / ValidPrescalers[TimerCode][PrescalerBits - 1]};
			else
				return {PrescalerBits + 1, F_CPU * Milliseconds / 1000 / ValidPrescalers[TimerCode][PrescalerBits]};
		};
		template <uint8_t TimerCode>
		TimerSetting GetTimerSetting(float Milliseconds)
		{
			constexpr uint8_t NPTC = NoPrescalers[TimerCode];
			uint8_t PrescalerBits = 0;
			for (; PrescalerBits < NPTC; ++PrescalerBits)
				if (Milliseconds < MaxMilliseconds[TimerCode][PrescalerBits])
					break;
			if (PrescalerBits == NPTC)
				return {PrescalerBits, F_CPU * Milliseconds / 1000 / ValidPrescalers[TimerCode][PrescalerBits - 1]};
			else
				return {PrescalerBits + 1, F_CPU * Milliseconds / 1000 / ValidPrescalers[TimerCode][PrescalerBits]};
		}
#pragma endregion
#pragma region 硬件寄存器
		template <uint8_t TimerCode>
		extern volatile uint8_t &TCCRA;
		template <uint8_t TimerCode>
		extern volatile uint8_t &TCCRB;
		template <uint8_t TimerCode>
		extern volatile uint8_t &TIMSK;
		template <uint8_t TimerCode>
		extern volatile uint8_t &TIFR;
		// TCNT有可能是uint8_t或uint16_t类型，因此不能直接取引用
		template <uint8_t TimerCode>
		uint16_t GetTCNT();
		template <uint8_t TimerCode>
		void SetTCNT(uint16_t TCNT);
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
#pragma endregion
#pragma region 通用内核
		template <uint8_t TimerCode>
		volatile uint32_t SR;
		template <uint8_t TimerCode>
		volatile int32_t LR;
		template <uint8_t TimerCode>
		void (*volatile TimerTask)();
#pragma endregion
#pragma region 全模板实现
		// Compa0表示自我重复，不切换
		template <uint8_t TimerCode, uint16_t SmallRepeats, bool InfiniteLr, void (*DoneCallback)()>
		void Compa0()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
					SR<TimerCode> = SmallRepeats;
				else
				{
					TIMSK<TimerCode> = 0;
					if (DoneCallback)
						DoneCallback();
				}
				TimerTask<TimerCode>();
			}
		}
		template <uint8_t TimerCode, uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr, void (*DoneCallback)()>
		void Compa2();
		// Compa1是小重复，结束后要切换到大重复，但是大重复有可能是COMPA也可能是OVF
		template <uint8_t TimerCode, uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr, void (*DoneCallback)()>
		void Compa1()
		{
			if (!--SR<TimerCode>)
			{
				if (Tcnt2 < TimerMax[TimerCode])
				{
					SetOCRA<TimerCode>(Tcnt2);
					COMPA<TimerCode> = Compa2<TimerCode, Tcnt1, Tcnt2, SR1, SR2, InfiniteLr, DoneCallback>;
				}
				else
					TIMSK<TimerCode> = 1;
				SR<TimerCode> = SR2;
			}
		}
		// Compa2是大重复，结束后要执行动作，如果有重复次数还要切换到小重复
		template <uint8_t TimerCode, uint16_t Tcnt1, uint16_t Tcnt2, uint16_t SR1, uint16_t SR2, bool InfiniteLr, void (*DoneCallback)()>
		void Compa2()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
				{
					if (Tcnt2 < TimerMax[TimerCode])
					{
						SetOCRA<TimerCode>(Tcnt1);
						COMPA<TimerCode> = Compa1<TimerCode, Tcnt1, Tcnt2, SR1, SR2, InfiniteLr, DoneCallback>;
					}
					else
						TIMSK<TimerCode> = 2;
					SR<TimerCode> = SR1;
				}
				else
				{
					TIMSK<TimerCode> = 0;
					if (DoneCallback)
						DoneCallback();
				}
				TimerTask<TimerCode>();
			}
		}
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, int32_t RepeatTimes, void (*DoneCallback)()>
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
				if (IdealRepeats * TM < TCNT || TimerCode == 0) // Timer0的OVF不可用
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					constexpr uint16_t ActualRepeats = IdealRepeats + 1;
					constexpr uint16_t Tcnt1 = TCNT / ActualRepeats;
					constexpr uint16_t Tcnt2 = Tcnt1 + 1;
					constexpr uint16_t SR1 = ActualRepeats * Tcnt2 - TCNT; // 必大于0
					constexpr uint16_t SR2 = TCNT - ActualRepeats * Tcnt1;
					SetOCRA<TimerCode>(Tcnt1);
					SR<TimerCode> = SR1;
					if (SR2)
					{
						COMPA<TimerCode> = Compa1<TimerCode, Tcnt1, Tcnt2, SR1, SR2, (RepeatTimes < 0), DoneCallback>;
						if (Tcnt2 == TM)
							OVF<TimerCode> = Compa2<TimerCode, Tcnt1, Tcnt2, SR1, SR2, (RepeatTimes < 0), DoneCallback>;
					}
					else
						COMPA<TimerCode> = Compa0<TimerCode, SR1, (RepeatTimes < 0), DoneCallback>;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 2;
				}
				else
				{
					SR<TimerCode> = IdealRepeats;
					OVF<TimerCode> = Compa0<TimerCode, IdealRepeats, (RepeatTimes < 0), DoneCallback>;
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 1;
				}
			}
		}
#pragma endregion
#pragma region 时长可变实现
		template <uint8_t TimerCode>
		volatile uint16_t Tcnt1;
		template <uint8_t TimerCode>
		volatile uint16_t Tcnt2;
		template <uint8_t TimerCode>
		volatile uint16_t SR1;
		template <uint8_t TimerCode>
		volatile uint16_t SR2;
		// Compa0表示自我重复，不切换
		template <uint8_t TimerCode, bool InfiniteLr, void (*DoneCallback)()>
		void Compa0()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
					SR<TimerCode> = SR1<TimerCode>;
				else
				{
					TIMSK<TimerCode> = 0;
					if (DoneCallback)
						DoneCallback();
				}
				TimerTask<TimerCode>();
			}
		}
		template <uint8_t TimerCode, bool InfiniteLr, bool UseOvf, void (*DoneCallback)()>
		void Compa2();
		// Compa1是小重复，结束后要切换到大重复，但是大重复有可能是COMPA也可能是OVF
		template <uint8_t TimerCode, bool InfiniteLr, bool UseOvf, void (*DoneCallback)()>
		void Compa1()
		{
			if (!--SR<TimerCode>)
			{
				if (UseOvf)
					TIMSK<TimerCode> = 1;
				else
				{
					SetOCRA<TimerCode>(Tcnt2<TimerCode>);
					COMPA<TimerCode> = Compa2<TimerCode, InfiniteLr, UseOvf, DoneCallback>;
				}
				SR<TimerCode> = SR2<TimerCode>;
			}
		}
		// Compa2是大重复，结束后要执行动作，如果有重复次数还要切换到小重复
		template <uint8_t TimerCode, bool InfiniteLr, bool UseOvf, void (*DoneCallback)()>
		void Compa2()
		{
			if (!--SR<TimerCode>)
			{
				if (InfiniteLr || --LR<TimerCode>)
				{
					if (UseOvf)
						TIMSK<TimerCode> = 2;
					else
					{
						SetOCRA<TimerCode>(Tcnt1<TimerCode>);
						COMPA<TimerCode> = Compa1<TimerCode, InfiniteLr, UseOvf, DoneCallback>;
					}
					SR<TimerCode> = SR1<TimerCode>;
				}
				else
				{
					TIMSK<TimerCode> = 0;
					if (DoneCallback)
						DoneCallback();
				}
				TimerTask<TimerCode>();
			}
		}
		template <uint8_t TimerCode, int32_t RepeatTimes, void (*DoneCallback)()>
		void SLRepeaterSet(uint32_t TCNT, uint8_t PrescalerBits)
		{
			TIMSK<TimerCode> = 0;
			if (RepeatTimes != 0)
			{
				constexpr uint32_t TM = TimerMax[TimerCode];
				SR1<TimerCode> = TCNT / (TM - (TimerCode == 0));
				constexpr bool Timer02 = TimerCode == 0 || TimerCode == 2;
				constexpr bool InfiniteRepeat = RepeatTimes < 0;
				if (Timer02)
					TCCRB<TimerCode> = PrescalerBits;
				else
					TCCRA<TimerCode> = 0;
				if (!InfiniteRepeat)
					LR<TimerCode> = RepeatTimes;
				if (SR1<TimerCode> * TM < TCNT || TimerCode == 0) // Timer0的OVF不可用
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					uint16_t ActualRepeats = SR1<TimerCode> + 1;
					Tcnt1<TimerCode> = TCNT / ActualRepeats;
					Tcnt2<TimerCode> = Tcnt1<TimerCode> + 1;
					SR1<TimerCode> = ActualRepeats * Tcnt2<TimerCode> - TCNT; // 必大于0
					SR2<TimerCode> = TCNT - ActualRepeats * Tcnt1<TimerCode>;
					SetOCRA<TimerCode>(Tcnt1<TimerCode>);
					SR<TimerCode> = SR1<TimerCode>;
					bool UseOvf = Tcnt2<TimerCode> == TM;
					if (SR2<TimerCode>)
					{
						COMPA<TimerCode> = UseOvf ? Compa1<TimerCode, InfiniteRepeat, true, DoneCallback> : Compa1<TimerCode, InfiniteRepeat, false, DoneCallback>;
						if (Tcnt2<TimerCode> == TM)
							OVF<TimerCode> = UseOvf ? Compa2<TimerCode, InfiniteRepeat, true, DoneCallback> : Compa2<TimerCode, InfiniteRepeat, false, DoneCallback>;
					}
					else
						COMPA<TimerCode> = Compa0<TimerCode, InfiniteRepeat, DoneCallback>;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 2;
				}
				else
				{
					SR<TimerCode> = SR1<TimerCode>;
					OVF<TimerCode> = Compa0<TimerCode, InfiniteRepeat, DoneCallback>;
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 1;
				}
			}
		}
#pragma endregion
#pragma region 重复次数可变实现
		template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoneCallback)()>
		void SLRepeaterSet(int32_t RepeatTimes)
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
				if (IdealRepeats * TM < TCNT || TimerCode == 0) // Timer0的OVF不可用
				{
					if (Timer02)
						TCCRA<TimerCode> = 2;
					else
						TCCRB<TimerCode> = PrescalerBits + 8;
					constexpr uint16_t ActualRepeats = IdealRepeats + 1;
					constexpr uint16_t Tcnt1 = TCNT / ActualRepeats;
					constexpr uint16_t Tcnt2 = Tcnt1 + 1;
					constexpr uint16_t SR1 = ActualRepeats * Tcnt2 - TCNT; // 必大于0
					constexpr uint16_t SR2 = TCNT - ActualRepeats * Tcnt1;
					SetOCRA<TimerCode>(Tcnt1);
					SR<TimerCode> = SR1;
					if (SR2)
					{
						COMPA<TimerCode> = RepeatTimes < 0 ? Compa1<TimerCode, Tcnt1, Tcnt2, SR1, SR2, true, DoneCallback> : Compa1<TimerCode, Tcnt1, Tcnt2, SR1, SR2, false, DoneCallback>;
						if (Tcnt2 == TM)
							OVF<TimerCode> = RepeatTimes < 0 ? Compa2<TimerCode, Tcnt1, Tcnt2, SR1, SR2, true, DoneCallback> : Compa2<TimerCode, Tcnt1, Tcnt2, SR1, SR2, false, DoneCallback>;
					}
					else
						COMPA<TimerCode> = RepeatTimes < 0 ? Compa0<TimerCode, SR1, true, DoneCallback> : Compa0<TimerCode, SR1, false, DoneCallback>;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 2;
				}
				else
				{
					SR<TimerCode> = IdealRepeats;
					OVF<TimerCode> = RepeatTimes < 0 ? Compa0<TimerCode, IdealRepeats, true, DoneCallback> : Compa0<TimerCode, IdealRepeats, false, DoneCallback>;
					if (Timer02)
						TCCRA<TimerCode> = 0;
					else
						TCCRB<TimerCode> = PrescalerBits;
					SetTCNT<TimerCode>(0);
					TIFR<TimerCode> = 255;
					TIMSK<TimerCode> = 1;
				}
			}
		}
#pragma endregion
	}
}
void TestFun();