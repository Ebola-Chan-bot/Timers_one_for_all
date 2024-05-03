#include "Timers_one_for_all_2.hpp"
using namespace Timers_one_for_all;
_TimerState Timers_one_for_all::_TimerStates[NumTimers];
const TimerClass *Timers_one_for_all::AllocateIdleTimer()
{
	for (int8_t T = NumTimers - 1; T >= 0; --T)
		if (!HardwareTimers[T]->Busy())
			return HardwareTimers[T];
	return nullptr;
}
template <uint8_t... Values>
using U8Sequence = std::integer_sequence<uint8_t, Values...>;
template <uint8_t First, typename T>
struct U8SequencePrepend;
template <uint8_t First, uint8_t... Rest>
struct U8SequencePrepend<First, U8Sequence<Rest...>>
{
	using type = U8Sequence<First, Rest...>;
};
template <typename T>
struct U8SequenceDiff;
template <uint8_t First, uint8_t Second, uint8_t... Rest>
struct U8SequenceDiff<U8Sequence<First, Second, Rest...>>
{
	using type = U8SequencePrepend<Second - First, U8SequenceDiff<U8Sequence<Second, Rest...>>::type>::type;
};
template <uint8_t First>
struct U8SequenceDiff<U8Sequence<First>>
{
	using type = U8Sequence<>;
};
// 输入预分频器应排除起始0
template <typename T, uint8_t BitIndex = 0, uint8_t PrescalerIndex = 1>
struct BitLimit
{
	using type = U8SequencePrepend<PrescalerIndex, BitLimit<T, BitIndex + 1, PrescalerIndex>::type>::type;
};
template <uint8_t BitIndex, uint8_t PrescalerIndex, uint8_t... Rest>
struct BitLimit<U8Sequence<BitIndex + 1, Rest...>, BitIndex, PrescalerIndex>
{
	using type = U8SequencePrepend<PrescalerIndex, BitLimit<U8Sequence<Rest...>, BitIndex + 1, PrescalerIndex + 1>::type>::type;
};
template <uint8_t BitIndex, uint8_t PrescalerIndex>
struct BitLimit<U8Sequence<BitIndex + 1>, BitIndex, PrescalerIndex>
{
	using type = U8Sequence<PrescalerIndex>;
};
template <typename T>
struct PrescalerDiff;
template <uint8_t... Diff>
struct PrescalerDiff<U8Sequence<Diff...>>
{
	static constexpr uint8_t AdvanceFactor[] = {1 << Diff...};
};
// 0号是无限大预分频器，表示暂停
template <uint8_t... BitShift>
struct PrescalerType : public PrescalerDiff<U8SequenceDiff<U8Sequence<BitShift...>>::type>
{
	static constexpr uint8_t BitShifts[] = {UINT8_MAX, BitShift...};
	static constexpr uint8_t NumPrescalers = sizeof...(BitShift) + 1;
};
#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0
ISR(TIMER0_COMPA_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].OVFCOMPA();
}
ISR(TIMER0_COMPB_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].COMPB();
}
#endif
#define TimerIsr(T)                                             \
	ISR(TIMER##T##_OVF_vect)                                    \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].OVFCOMPA(); \
	}                                                           \
	ISR(TIMER##T##_COMPA_vect)                                  \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].OVFCOMPA(); \
	}                                                           \
	ISR(TIMER##T##_COMPB_vect)                                  \
	{                                                           \
		_TimerStates[(size_t)TimerEnum::Timer##T##].COMPB();    \
	}
#ifdef TOFA_TIMER1
TimerIsr(1);
#endif
#ifdef TOFA_TIMER2
#warning 使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2
TimerIsr(2);
#endif
#ifdef TOFA_TIMER3
TimerIsr(3);
#endif
#ifdef TOFA_TIMER4
TimerIsr(4);
#endif
#ifdef TOFA_TIMER5
TimerIsr(5);
#endif
using Prescaler01 = PrescalerType<0, 3, 6, 8, 10>;
using Prescaler2 = PrescalerType<0, 3, 5, 6, 7, 8, 10>;
void TimerClass::Pause() const
{
	const uint8_t Clock = TCCRB & 0b111;
	if (Clock)
	{
		State.Clock = Clock;
		TCCRB &= ~0b111;
	}
}
void TimerClass::Continue() const
{
	if (!(TCCRB & 0b111))
		TCCRB |= State.Clock;
}
template <typename PrescalerType>
void StartTiming(TimerClass *Timer)
{
	Timer->TIFR = -1;
	interrupts();
	Timer->State.OverflowCount = 0;
	Timer->State.OverflowTarget = PrescalerType::AdvanceFactor[0];
	Timer->State.OVFCOMPA = [Timer]()
	{
		_TimerState &State = Timer->State;
		uint32_t &OverflowCount = State.OverflowCount;
		if (++OverflowCount == State.OverflowTarget)
		{
			const uint8_t TCCRB = Timer->TCCRB++;
			OverflowCount = 1;
			if ((TCCRB < PrescalerType::NumPrescalers))
				State.OverflowTarget = PrescalerType::AdvanceFactor[TCCRB];
			else
				State.OVFCOMPA = [&OverflowCount]()
				{ OverflowCount++; };
		}
	};
}
void TimerClass0::StartTiming() const
{
	noInterrupts();
	TCNT0 = 0;
	TCCR0B = 1;
	TCCR0A = 0;
	OCR0A = 0;
	TIMSK0 = 1 << OCIE0A;
	::StartTiming<Prescaler01>((TimerClass *)this);
}
void TimerClass1::StartTiming() const
{
	noInterrupts();
	TCNT = 0;
	TCCRB = 1;
	TCCRA = 0;
	TIMSK = 1 << TOIE1;
	::StartTiming<Prescaler01>((TimerClass *)this);
}
void TimerClass2::StartTiming() const
{
	noInterrupts();
	TCNT2 = 0;
	TCCR2B = 1;
	TCCR2A = 0;
	TIMSK2 = 1 << TOIE1;
	::StartTiming<Prescaler2>((TimerClass *)this);
}
Tick TimerClass0::GetTiming() const
{
	return Tick((State.OverflowCount << 8) + TCNT0 << Prescaler01::BitShifts[TCCR0B]);
}
Tick TimerClass1::GetTiming() const
{
	return Tick((State.OverflowCount << 16) + TCNT << Prescaler01::BitShifts[TCCRB]);
}
Tick TimerClass2::GetTiming() const
{
	return Tick((State.OverflowCount << 8) + TCNT2 << Prescaler2::BitShifts[TCCR2B]);
}
void TimerClass0::Delay(Tick Time) const
{
}
#endif