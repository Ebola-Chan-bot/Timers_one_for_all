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
template <typename T>
struct BitsToPrescaler_s;
template <uint8_t... Bit>
struct BitsToPrescaler_s<U8Sequence<Bit...>>
{
	static constexpr uint8_t BitsToPrescaler[] = {Bit...};
};
// 0号是无限大预分频器，表示暂停
template <uint8_t... BitShift>
struct PrescalerType : public PrescalerDiff<U8SequenceDiff<U8Sequence<0, BitShift...>>::type>, public BitsToPrescaler_s<BitLimit<U8Sequence<BitShift...>>::type>
{
	static constexpr uint8_t BitShifts[] = {-1, 0, BitShift...};
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
using Prescaler01 = PrescalerType<3, 6, 8, 10>;
using Prescaler2 = PrescalerType<3, 5, 6, 7, 8, 10>;
template <typename PrescalerType>
void StartTiming(TimerClass *Timer)
{
	Timer->TIFR = -1;
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
			if ((TCCRB < std::extent_v<decltype(PrescalerType::AdvanceFactor)>))
				State.OverflowTarget = PrescalerType::AdvanceFactor[TCCRB];
			else
				State.OVFCOMPA = [&OverflowCount]()
				{ OverflowCount++; };
		}
	};
}
void TimerClass0::StartTiming() const
{
	TCNT0 = 0;
	TCCR0B = 1;
	TCCR0A = 0;
	OCR0A = 0;
	TIMSK0 = 1 << OCIE0A;
	::StartTiming<Prescaler01>((TimerClass *)this);
}
void TimerClass1::StartTiming() const
{
	TCNT = 0;
	TCCRB = 1;
	TCCRA = 0;
	TIMSK = 1 << TOIE1;
	::StartTiming<Prescaler01>((TimerClass *)this);
}
void TimerClass2::StartTiming() const
{
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
// 返回OverflowTarget
template <typename Prescaler>
uint32_t PrescalerOverflow(uint32_t Cycles, volatile uint8_t &Clock)
{
	constexpr uint8_t MaxBits = std::extent_v<decltype(Prescaler::BitsToPrescaler)>;
	const uint8_t CycleBits = sizeof(Cycles) * 8 - 1 - __builtin_clz(Cycles);
	if (CycleBits < MaxBits)
	{
		Clock = Prescaler::BitsToPrescaler[CycleBits];
		return 0;
	}
	else
		return Cycles >> Prescaler::BitShifts[Clock = Prescaler::BitsToPrescaler[MaxBits - 1]];
}
void TimerClass0::Delay(Tick Time) const
{
	TCCR0A = 0;
	TIFR0 = -1;
	TCNT0 = 0;
	uint8_t WaitFor;
	if (State.OverflowTarget = PrescalerOverflow<Prescaler01>(Time.count() >> 8, TCCR0B))
	{
		OCR0A = 0;
		WaitFor = (Time.count() >> Prescaler01::BitShifts[std::extent_v<decltype(Prescaler01::BitShifts)> - 1]) & UINT8_MAX;
		volatile uint32_t &OverflowCount = State.OverflowCount;
		OverflowCount = 0;
		State.OVFCOMPA = [&OverflowCount]()
		{ ++OverflowCount; };
		TIMSK0 = 1 << OCIE0A;
		while (OverflowCount < State.OverflowTarget)
			;
	}
	else
		WaitFor = Time.count() >> Prescaler01::BitShifts[TCCRB];
	TIMSK0 = 0;
	while (TCNT0 < WaitFor)
		;
}
void TimerClass1::Delay(Tick Time) const
{
	TCCRA = 0;
	TIFR = -1;
	TCNT = 0;
	uint16_t WaitFor;
	if (State.OverflowTarget = PrescalerOverflow<Prescaler01>(Time.count() >> 16, TCCRB))
	{
		WaitFor = (Time.count() >> Prescaler01::BitShifts[std::extent_v<decltype(Prescaler01::BitShifts)> - 1]) & UINT16_MAX;
		volatile uint32_t &OverflowCount = State.OverflowCount;
		OverflowCount = 0;
		State.OVFCOMPA = [&OverflowCount]()
		{ ++OverflowCount; };
		TIMSK = 1 << TOIE1;
		while (OverflowCount < State.OverflowTarget)
			;
	}
	else
		WaitFor = Time.count() >> Prescaler01::BitShifts[TCCRB];
	TIMSK = 0;
	while (TCNT < WaitFor)
		;
}
void TimerClass2::Delay(Tick Time) const
{
	TCCR2A = 0;
	TIFR2 = -1;
	TCNT2 = 0;
	uint8_t WaitFor;
	if (State.OverflowTarget = PrescalerOverflow<Prescaler2>(Time.count() >> 8, TCCR2B))
	{
		WaitFor = (Time.count() >> Prescaler2::BitShifts[std::extent_v<decltype(Prescaler2::BitShifts)> - 1]) & UINT8_MAX;
		volatile uint32_t &OverflowCount = State.OverflowCount;
		OverflowCount = 0;
		State.OVFCOMPA = [&OverflowCount]()
		{ ++OverflowCount; };
		TIMSK0 = 1 << TOIE2;
		while (OverflowCount < State.OverflowTarget)
			;
	}
	else
		WaitFor = Time.count() >> Prescaler2::BitShifts[TCCRB];
	TIMSK2 = 0;
	while (TCNT2 < WaitFor)
		;
}
void TimerClass0::DoAfter(Tick After, std::function<void()> Do) const
{
	TCCR0A = 0;
	TIFR0 = -1;
	TCNT0 = 0;
	if (State.OverflowTarget = PrescalerOverflow<Prescaler01>(After.count() >> 8, TCCR0B))
	{
		OCR0A = 0;
		_TimerState &StateReference = State;
		StateReference.OverflowCount = 0;
		StateReference.OVFCOMPA = [&StateReference]()
		{
			if (++StateReference.OverflowCount >=StateReference.OverflowTarget)
			
		};
	}
}
#endif