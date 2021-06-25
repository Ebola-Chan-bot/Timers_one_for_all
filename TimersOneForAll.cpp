#include "TimersOneForAll.h"
struct HardwarePrescaler
{
	uint8_t PSBits;
	uint16_t PSValue;
};
struct TimerSetting
{
	uint8_t PrescalerBits;
	uint32_t TCNT;
};
constexpr uint32_t TimerMax[] = {256, 65536, 256, 65536, 65536, 65536};
constexpr TimerSetting SelectPrescaler(float Milliseconds, uint8_t TimerCode, const HardwarePrescaler ValidPrescalers[], uint8_t NoVPs)
{
	float NoOscillations = F_CPU / 1000 * Milliseconds;
	float IdealPrescaler = NoOscillations / TimerMax[TimerCode];
	HardwarePrescaler PS = {};
	for (uint8_t a = 0; a < NoVPs; ++a)
	{
		PS = ValidPrescalers[a];
		if (PS.PSValue > IdealPrescaler)
			break;
	}
	return {PS.PSBits, NoOscillations / PS.PSValue};
};
template <uint8_t TimerCode>
constexpr TimerSetting GetTimerSetting(float Miliseconds)
{
	constexpr HardwarePrescaler ValidPrescalers[] = {{0b001, 1}, {0b010, 8}, {0b011, 64}, {0b100, 256}, {0b101, 1024}};
	constexpr HardwarePrescaler ValidPrescalers2[] = {{0b001, 1}, {0b010, 8}, {0b011, 32}, {0b100, 64}, {0b101, 128}, {0b110, 256}, {0b111, 1024}};
	return SelectPrescaler(Miliseconds, TimerCode, TimerCode == 2 ? ValidPrescalers2 : ValidPrescalers, 5);
};
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
volatile void (*OVF)();
template <uint8_t TimerCode>
volatile void (*COMPA)();
template <uint8_t TimerCode>
volatile void (*COMPB)();
template <uint8_t PinCode>
void EfficientDigitalToggle()
{
	uint8_t *const PinPort = portOutputRegister(digitalPinToPort(PinCode));
	const uint8_t BitMask = digitalPinToBitMask(PinCode);
	*PinPort ^= BitMask;
}
template <uint8_t PinCode, bool IsHigh>
void EfficientDigitalWrite()
{
	uint8_t *const PinPort = portOutputRegister(digitalPinToPort(PinCode));
	const uint8_t BitMask = digitalPinToBitMask(PinCode);
	if (IsHigh)
		*PinPort |= BitMask;
	else
		*PinPort &= ~BitMask;
}
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
	ISR(TIMER##Code##_OVF_vect)                    \
	{                                              \
		OVF<Code>();                               \
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
#endif
#ifdef TCNT1
TimerSpecialize(1);
#endif
#ifdef TCNT2
TimerSpecialize(2);
#endif
#ifdef TCNT3
TimerSpecialize(3);
#endif
#ifdef TCNT4
TimerSpecialize(4);
#endif
#ifdef TCNT5
TimerSpecialize(5);
#endif
template <uint8_t TimerCode>
uint16_t IdealRepeats;
template <uint8_t TimerCode>
uint16_t SmallRepeats;
template <uint8_t TimerCode>
uint16_t LargeRepeats;
template <uint8_t TimerCode>
uint16_t TCNTSmall;
template <uint8_t TimerCode>
uint16_t TCNTLarge;
template <uint8_t TimerCode>
uint16_t IR;
template <uint8_t TimerCode>
uint16_t SR;
template <uint8_t TimerCode>
uint16_t LR;
template <uint8_t TimerCode, void (*DoTask)()>
void COMPALarge();
template <uint8_t TimerCode, void (*DoTask)()>
void COMPASmall()
{
	if (!--SR<TimerCode>)
	{
		LR<TimerCode> = LargeRepeats<TimerCode>;
		SetOCRA<TimerCode>(TCNTLarge<TimerCode>);
		COMPA<TimerCode> = COMPALarge<TimerCode, DoTask>;
	}
};
template <uint8_t TimerCode, void (*DoTask)()>
void COMPALarge()
{
	if (!--LR<TimerCode>)
	{
		SR<TimerCode> = SmallRepeats<TimerCode>;
		SetOCRA<TimerCode>(TCNTSmall<TimerCode>);
		COMPA<TimerCode> = COMPASmall<TimerCode, DoTask>;
		DoTask();
	}
};
template <uint8_t TimerCode, void (*DoTask)()>
void COMPALargeRepeat();
template <uint8_t TimerCode, void (*DoTask)()>
void COMPASmallRepeat()
{
	if (!--SR<TimerCode>)
	{
		LR<TimerCode> = LargeRepeats<TimerCode>;
		SetOCRA<TimerCode>(TCNTLarge<TimerCode>);
		COMPA<TimerCode> = COMPALargeRepeat<TimerCode, DoTask>;
	}
};
template <uint8_t TimerCode, void (*DoTask)()>
void COMPALargeRepeat()
{
	if (!--LR<TimerCode>)
	{
		if (--RT<TimerCode>)
		{
			SR<TimerCode> = SmallRepeats<TimerCode>;
			SetOCRA<TimerCode>(TCNTSmall<TimerCode>);
			COMPA<TimerCode> = COMPASmallRepeat<TimerCode, DoTask>;
		}
		else
			TIMSK<TimerCode> = 0;
		DoTask();
	}
};
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighTCNT, uint16_t LowTCNT>
void COMPASquareWave()
{
	SetORCA<TimerCode>(GetORCA<TimerCode>() == HighTCNT ? LowTCNT : HighTCNT);
	EfficientDigitalToggle<PinCode>();
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighTCNT, uint16_t LowTCNT>
void COMPASquareWaveRepeat()
{
	SetORCA<TimerCode>(GetORCA<TimerCode>() == HighTCNT ? LowTCNT : HighTCNT);
	EfficientDigitalToggle<PinCode>();
	if (!--RT<TimerCode>)
		TIMSK<TimerCode> = 0;
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWaveToLow();
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWaveToHigh()
{
	EfficientDigitalWrite<PinCode, HIGH>();
	TimersOneForAll::DoAfter<TimerCode, HighMilliseconds, SquareWaveToLow<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>>();
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWaveToLow()
{
	EfficientDigitalWrite<PinCode, LOW>();
	TimersOneForAll::DoAfter<TimerCode, LowMilliseconds, SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>>();
}
template <uint8_t TimerCode>
uint8_t RT;
template <uint8_t TimerCode>
uint16_t HighTCNT;
template <uint8_t TimerCode>
uint16_t LowTCNT;
template <uint8_t TimerCode, uint16_t AfterMillisecondes, void (*DoTask)()>
void TimersOneForAll::DoAfter()
{
	if (AfterMillisecondes)
	{
		constexpr TimerSetting TS = GetTimerSetting<TimerCode>(AfterMillisecondes);
		constexpr uint32_t TM = TimerMax[TimerCode];
		TIMSK<TimerCode> = 0;
		TCCRB<TimerCode> = TS.PrescalerBits;
		//拒绝外部振荡源
		bitWrite(ASSR, AS2, false);
		if (TS.TCNT < TM)
		{
			SetOCRA<TimerCode>(TS.TCNT);
			COMPA<TimerCode> = []
			{
				TIMSK<TimerCode> = 0;
				DoTask();
			};
			TCCRA<TimerCode> = 0;
			SetTCNT<TimerCode>(0);
			TIMSK<TimerCode> = 2;
		}
		else
		{
			IdealRepeats<TimerCode> = TS.TCNT / TM;
			if (uint32_t(IdealRepeats<TimerCode>) * TM < TS.TCNT)
			{
				constexpr uint32_t ActualRepeats = IdealRepeats<TimerCode> + 1;
				TCNTSmall<TimerCode> = TS.TCNT / ActualRepeats;
				TCNTLarge<TimerCode> = TCNTSmall<TimerCode> + 1;
				SmallRepeats<TimerCode> = ActualRepeats * TCNTLarge<TimerCode> - TS.TCNT;
				LargeRepeats<TimerCode> = TS.TCNT - ActualRepeats * TCNTSmall<TimerCode>;
				SetOCRA<TimerCode>(TCNTSmall<TimerCode>);
				COMPA<TimerCode> = []
				{
					if (!--SmallRepeats<TimerCode>)
					{
						SetOCRA<TimerCode>(TCNTLarge<TimerCode>);
						COMPA<TimerCode> = []
						{
							if (!--LargeRepeats<TimerCode>)
							{
								TIMSK<TimerCode> = 0;
								DoTask();
							}
						}
					}
				};
				TCCRA<TimerCode> = 2;
				SetTCNT<TimerCode>(0);
				TIMSK<TimerCode> = 2;
			}
			else
			{
				IR<TimerCode> = IdealRepeats<TimerCode>;
				OVF<TimerCode> = []
				{
					if (!--IR<TimerCode>)
					{
						TIMSK<TimerCode> = 0;
						DoTask();
					}
				};
				TCCRA<TimerCode> = 0;
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
	TCCRB<TimerCode> = PrescalerBits;
	//拒绝外部振荡源
	bitWrite(ASSR, AS2, false);
	if (TCNT < TM)
	{
		SetOCRA<TimerCode>(TCNT);
		COMPA<TimerCode> = DoTask;
		TCCRA<TimerCode> = 2;
		SetTCNT<TimerCode>(0);
		TIMSK<TimerCode> = 2;
	}
	else
	{
		IdealRepeats<TimerCode> = TCNT / TM;
		if (uint32_t(IdealRepeats<TimerCode>) * TM < TCNT)
		{
			constexpr uint32_t ActualRepeats = IdealRepeats<TimerCode> + 1;
			TCNTSmall<TimerCode> = TCNT / ActualRepeats;
			TCNTLarge<TimerCode> = TCNTSmall<TimerCode> + 1;
			SmallRepeats<TimerCode> = ActualRepeats * TCNTLarge<TimerCode> - TCNT;
			LargeRepeats<TimerCode> = TCNT - ActualRepeats * TCNTSmall<TimerCode>;
			SR<TimerCode> = SmallRepeats<TimerCode>;
			SetOCRA<TimerCode>(TCNTSmall<TimerCode>);
			COMPA<TimerCode> = COMPASmall<TimerCode, DoTask>;
			TCCRA<TimerCode> = 2;
			SetTCNT<TimerCode>(0);
			TIMSK<TimerCode> = 2;
		}
		else
		{
			IR<TimerCode> = IdealRepeats<TimerCode>;
			OVF<TimerCode> = []
			{
				if (!--IR<TimerCode>)
				{
					IR<TimerCode> = IdealRepeats<TimerCode>;
					DoTask();
				}
			};
			TCCRA<TimerCode> = 0;
			SetTCNT<TimerCode>(0);
			TIMSK<TimerCode> = 1;
		}
	}
}
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)()>
void RepeatAfter()
{
	constexpr TimerSetting TS = GetTimerSetting<TimerCode>(IntervalMillisecondes);
	InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask>();
}
template <uint8_t TimerCode, uint32_t TCNT, uint8_t PrescalerBits, void (*DoTask)(), uint8_t RepeatTimes>
void InternalRA()
{
	switch (RepeatTimes)
	{
	case 0:
		TIMSK<TimerCode> = 0;
		break;
	case 1:
		TimersOneForAll::DoAfter<TimerCode, IntervalMilliseconds, DoTask>();
		break;
	default:
		constexpr uint32_t TM = TimerMax[TimerCode];
		TIMSK<TimerCode> = 0;
		TCCRB<TimerCode> = PrescalerBits;
		//拒绝外部振荡源
		bitWrite(ASSR, AS2, false);
		RT<TimerCode> = RepeatTimes;
		if (TCNT < TM)
		{
			SetOCRA<TimerCode>(TCNT);
			COMPA<TimerCode> = []
			{
				if (!--RT<TimerCode>)
					TIMSK<TimerCode> = 0;
				DoTask();
			};
			TCCRA<TimerCode> = 2;
			SetTCNT<TimerCode>(0);
			TIMSK<TimerCode> = 2;
		}
		else
		{
			IdealRepeats<TimerCode> = TCNT / TM;
			if (uint32_t(IdealRepeats<TimerCode>) * TM < TCNT)
			{
				constexpr uint32_t ActualRepeats = IdealRepeats<TimerCode> + 1;
				TCNTSmall<TimerCode> = TCNT / ActualRepeats;
				TCNTLarge<TimerCode> = TCNTSmall<TimerCode> + 1;
				SmallRepeats<TimerCode> = ActualRepeats * TCNTLarge<TimerCode> - TCNT;
				LargeRepeats<TimerCode> = TCNT - ActualRepeats * TCNTSmall<TimerCode>;
				SR<TimerCode> = SmallRepeats<TimerCode>;
				SetOCRA<TimerCode>(TCNTSmall<TimerCode>);
				COMPA<TimerCode> = COMPASmallRepeat<TimerCode, DoTask>;
				TCCRA<TimerCode> = 2;
				SetTCNT<TimerCode>(0);
				TIMSK<TimerCode> = 2;
			}
			else
			{
				IR<TimerCode> = IdealRepeats<TimerCode>;
				OVF<TimerCode> = []
				{
					if (!--IR<TimerCode>)
					{
						if (--RT<TimerCode>)
							IR<TimerCode> = IdealRepeats<TimerCode>;
						else
							TIMSK<TimerCode> = 0;
						DoTask();
					}
				};
				TCCRA<TimerCode> = 0;
				SetTCNT<TimerCode>(0);
				TIMSK<TimerCode> = 1;
			}
		}
	}
}
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), uint8_t RepeatTimes>
void RepeatAfter()
{
	constexpr TimerSetting TS = GetTimerSetting<TimerCode>(IntervalMillisecondes);
	InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, DoTask, RepeatTimes>();
}
template <uint8_t TimerCode>
void TimersOneForAll::StartTiming()
{
	MillisecondsElapsed<TimerCode> = 0;
	RepeatAfter<TimerCode, 1, []
				{ MillisecondsElapsed<TimerCode> ++; }>();
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
void PlayTone()
{
	constexpr TimerSetting TS = GetTimerSetting<TimerCode>(500.f / FrequencyHz);
	EfficientDigitalToggle<PinCode>();
	InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>>();
}
//在指定引脚上生成指定频率的方波，持续指定的毫秒数
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds>
void PlayTone()
{
	constexpr TimerSetting TS = GetTimerSetting<TimerCode>(500.f / FrequencyHz);
	EfficientDigitalToggle<PinCode>();
	InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>, uint32_t(FrequencyHz) * Milliseconds / 1000>();
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWave()
{
	if (HighMilliseconds == LowMilliseconds)
	{
		constexpr TimerSetting TS = GetTimerSetting<TimerCode>(HighMilliseconds);
		EfficientDigitalWrite<PinCode, HIGH>();
		InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>>();
	}
	else
	{
		constexpr uint32_t TM = TimerMax[TimerCode];
		constexpr uint32_t FullCycle = HighMilliseconds + LowMilliseconds;
		constexpr TimerSetting TryTS = GetTimerSetting<TimerCode>(FullCycle);
		if (TryTS.TCNT < TM)
		{
			TIMSK<TimerCode> = 0;
			TCCRA<TimerCode> = 2;
			TCCRB<TimerCode> = TryTS.PrescalerBits;
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
			constexpr TimerSetting TS = GetTimerSetting<TimerCode>(LongMilliseconds);
			if (TS.TCNT < TM)
			{
				TIMSK<TimerCode> = 0;
				TCCRA<TimerCode> = 2;
				TCCRB<TimerCode> = TS.PrescalerBits;
				constexpr uint16_t ShortTCNT = uint32_t(TS.TCNT) * ShortMilliseconds / LongMilliseconds;
				constexpr uint16_t HighTCNT = HGL ? TS.TCNT : ShortTCNT;
				constexpr uint16_t LowTCNT = HGL ? ShortTCNT : TS.TCNT;
				SetOCRA<TimerCode>(HighTCNT);
				COMPA<TimerCode> = COMPASquareWave<TimerCode, PinCode, HighTCNT, LowTCNT>;
				EfficientDigitalWrite<PinCode, HIGH>();
				SetTCNT<TimerCode>(0);
				TIMSK<TimerCode> = 2;
			}
			else
				SquareWaveToHigh<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>();
		}
	}
}
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, uint8_t RepeatTimes>
void SquareWave()
{
	switch (RepeatTimes)
	{
	case 0:
		TIMSK<TimerCode> = 0;
		break;
	case 1:
		EfficientDigitalWrite<PinCode, HIGH>();
		TimersOneForAll::DoAfter<TimerCode, HighMilliseconds, EfficientDigitalWrite<PinCode, LOW>>();
		break;
	default:
		if (HighMilliseconds == LowMilliseconds)
		{
			constexpr TimerSetting TS = GetTimerSetting<TimerCode>(HighMilliseconds);
			EfficientDigitalWrite<PinCode, HIGH>();
			InternalRA<TimerCode, TS.TCNT, TS.PrescalerBits, EfficientDigitalToggle<PinCode>, RepeatTimes * 2 - 1>();
		}
		else
		{
			constexpr uint32_t TM = TimerMax[TimerCode];
			constexpr uint32_t FullCycle = HighMilliseconds + LowMilliseconds;
			constexpr TimerSetting TryTS = GetTimerSetting<TimerCode>(FullCycle);
			if (TryTS.TCNT < TM)
			{
				TIMSK<TimerCode> = 0;
				TCCRA<TimerCode> = 2;
				TCCRB<TimerCode> = TryTS.PrescalerBits;
				SetOCRA<TimerCode>(TryTS.TCNT);
				SetOCRB<TimerCode>(TryTS.TCNT * HighMilliseconds / FullCycle);
				RT<TimerCode> = RepeatTimes;
				COMPA<TimerCode> = EfficientDigitalWrite<PinCode, HIGH>;
				COMPB<TimerCode> = []
				{
					EfficientDigitalWrite<PinCode, LOW>();
					if (!--RT<TimerCode>)
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
				constexpr TimerSetting TS = GetTimerSetting<TimerCode>(LongMilliseconds);
				if (TS.TCNT < TM)
				{
					TIMSK<TimerCode> = 0;
					TCCRA<TimerCode> = 2;
					TCCRB<TimerCode> = TS.PrescalerBits;
					constexpr uint16_t ShortTCNT = uint32_t(TS.TCNT) * ShortMilliseconds / LongMilliseconds;
					constexpr uint16_t HighTCNT = HGL ? TS.TCNT : ShortTCNT;
					constexpr uint16_t LowTCNT = HGL ? ShortTCNT : TS.TCNT;
					SetOCRA<TimerCode>(HighTCNT);
					RT<TimerCode> = RepeatTimes * 2 - 1;
					COMPA<TimerCode> = COMPASquareWaveRepeat<TimerCode, PinCode, HighTCNT, LowTCNT>;
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
template <uint8_t TimerCode>
void ShutDown()
{
	TIMSK<TimerCode> = 0;
}