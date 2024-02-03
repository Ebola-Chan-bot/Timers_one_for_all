#include "Kernel.h"
#ifdef ARDUINO_ARCH_AVR
#define TimerSpecialize(Code)                      \
	template <>                                    \
	volatile uint8_t &TCCRA<Code> = TCCR##Code##A; \
	template <>                                    \
	volatile uint8_t &TCCRB<Code> = TCCR##Code##B; \
	template <>                                    \
	volatile uint8_t &TIMSK<Code> = TIMSK##Code;   \
	template <>                                    \
	volatile uint8_t &TIFR<Code> = TIFR##Code;     \
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
namespace TimersOneForAll
{
	namespace Internal
	{
#ifdef TCNT0
		TimerSpecialize(0);
// Timer0的溢出中断被内置millis()函数占用了，无法使用
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
	}
}
#endif
#ifdef ARDUINO_ARCH_SAM

#endif