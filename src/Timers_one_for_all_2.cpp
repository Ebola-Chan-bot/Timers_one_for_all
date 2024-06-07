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
	static constexpr uint8_t MaxClock = std::extent_v<decltype(BitShifts)> - 1;
	static constexpr uint8_t MaxShift = BitShifts[MaxClock];
};
#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0
ISR(TIMER0_COMPA_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].COMPA();
}
ISR(TIMER0_COMPB_vect)
{
	_TimerStates[(size_t)TimerEnum::Timer0].COMPB();
}
#endif
#define TimerIsr(T)                                          \
	ISR(TIMER##T##_OVF_vect)                                 \
	{                                                        \
		_TimerStates[(size_t)TimerEnum::Timer##T##].OVF();   \
	}                                                        \
	ISR(TIMER##T##_COMPA_vect)                               \
	{                                                        \
		_TimerStates[(size_t)TimerEnum::Timer##T##].COMPA(); \
	}                                                        \
	ISR(TIMER##T##_COMPB_vect)                               \
	{                                                        \
		_TimerStates[(size_t)TimerEnum::Timer##T##].COMPB(); \
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
	TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
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
	if (volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler01>(Time.count() >> 8, TCCR0B))
	{
		OCR0A = 0;
		State.COMPA = [&OverflowCount]()
		{ --OverflowCount; };
		TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
		while (OverflowCount)
			;
	}
	WaitFor = Time.count() >> Prescaler01::BitShifts[TCCRB];
	TIMSK0 = 1 << TOIE0;
	while (TCNT0 < WaitFor)
		;
}
void TimerClass1::Delay(Tick Time) const
{
	TCCRA = 0;
	TIFR = -1;
	TCNT = 0;
	uint16_t WaitFor;
	if (volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler01>(Time.count() >> 16, TCCRB))
	{
		State.OVF = [&OverflowCount]()
		{ --OverflowCount; };
		TIMSK = 1 << TOIE1;
		while (OverflowCount)
			;
	}
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
	if (volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler2>(Time.count() >> 8, TCCR2B))
	{
		State.OVF = [&OverflowCount]()
		{ --OverflowCount; };
		TIMSK2 = 1 << TOIE2;
		while (OverflowCount)
			;
	}
	WaitFor = Time.count() >> Prescaler2::BitShifts[TCCRB];
	TIMSK2 = 0;
	while (TCNT2 < WaitFor)
		;
}
void TimerClass0::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCNT0 = 0;
		if (State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 8, TCCR0B))
		{
			OCR0A = 0;
			State.COMPA = [this]()
			{
				if (!--State.OverflowCount)
					TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
			};
			TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
		}
		else
			TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
		OCR0B = After.count() >> Prescaler01::BitShifts[TCCR0B];
		State.COMPB = [Do]()
		{
			TIMSK0 = 1 << TOIE0;
			Do();
		};
		TCCR0A = 0;
		TIFR0 = -1;
	}
	else
		Do();
}
void TimerClass1::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCCRA = 0;
		TIFR = -1;
		TCNT = 0;
		if (State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 16, TCCRB))
		{
			State.OVF = [this]()
			{
				if (!--State.OverflowCount)
					TIMSK = 1 << OCIE1B;
			};
			TIMSK = 1 << TOIE1;
		}
		else
			TIMSK = 1 << OCIE1B;
		OCRB = After.count() >> Prescaler01::BitShifts[TCCRB];
		State.COMPB = [this, Do]()
		{
			TIMSK = 0;
			Do();
		};
	}
	else
		Do();
}
void TimerClass2::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCCR2A = 0;
		TIFR2 = -1;
		TCNT2 = 0;
		if (State.OverflowCount = PrescalerOverflow<Prescaler2>(After.count() >> 8, TCCR2B))
		{
			State.OVF = [this]()
			{
				if (!--State.OverflowCount)
					TIMSK2 = 1 << OCIE2B;
			};
			TIMSK2 = 1 << TOIE2;
		}
		else
			TIMSK2 = 1 << OCIE2B;
		OCR2B = After.count() >> Prescaler2::BitShifts[TCCR2B];
		State.COMPB = [Do]()
		{
			TIMSK2 = 0;
			Do();
		};
	}
	else
		Do();
}
void TimerClass0::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		TCNT0 = 0;
		if (const uint32_t OverflowTarget = PrescalerOverflow<Prescaler01>(Every.count() >> 8, TCCR0B))
		{
			OCR0A = (Every.count() >> Prescaler01::MaxShift);
			OCR0B = 0;
			State.OverflowCount = OverflowTarget;
			State.COMPB = [this]()
			{
				if (!--State.OverflowCount)
				{
					TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
					TCCR0A = 1 << WGM01;
				}
			};
			State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCR0A = 0;
				if (--State.RepeatLeft)
				{
					State.OverflowCount = OverflowTarget;
					TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
				}
				else
					TIMSK0 = 1 << TOIE0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 0;
			TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
		}
		else
		{
			OCR0A = Every.count() >> Prescaler01::BitShifts[TCCR0B];
			State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 1 << WGM01;
			TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
		}
		TIFR0 = -1;
		State.RepeatLeft = RepeatTimes;
	}
	else
		DoneCallback();
}
void TimerClass1::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		TCNT = 0;
		if (const uint32_t OverflowTarget = PrescalerOverflow<Prescaler01>(Every.count() >> 16, TCCRB))
		{
			OCRA = (Every.count() >> Prescaler01::MaxShift);
			State.OverflowCount = OverflowTarget;
			State.OVF = [this]()
			{
				if (!--State.OverflowCount)
				{
					TIMSK = 1 << OCIE1A;
					TCCRB = (1 << WGM12) | Prescaler01::MaxClock;
				}
			};
			State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCRB = Prescaler01::MaxClock;
				if (--State.RepeatLeft)
				{
					State.OverflowCount = OverflowTarget;
					TIMSK = 1 << TOIE1;
				}
				else
					TIMSK = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TIMSK = 1 << TOIE1;
		}
		else
		{
			OCRA = Every.count() >> Prescaler01::BitShifts[TCCRB];
			State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCRB |= 1 << WGM12;
			TIMSK = 1 << OCIE1A;
		}
		TCCRA = 0;
		TIFR = -1;
		State.RepeatLeft = RepeatTimes;
	}
	else
		DoneCallback();
}
void TimerClass2::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		TCNT2 = 0;
		if (const uint32_t OverflowTarget = PrescalerOverflow<Prescaler2>(Every.count() >> 8, TCCR2B))
		{
			OCR2A = (Every.count() >> Prescaler2::MaxShift);
			State.OverflowCount = OverflowTarget;
			State.OVF = [this]()
			{
				if (!--State.OverflowCount)
				{
					TIMSK2 = 1 << OCIE2A;
					TCCR2A = 1 << WGM21;
				}
			};
			State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCR2A = 0;
				if (--State.RepeatLeft)
				{
					State.OverflowCount = OverflowTarget;
					TIMSK2 = 1 << TOIE2;
				}
				else
					TIMSK2 = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 0;
			TIMSK2 = 1 << TOIE2;
		}
		else
		{
			OCR2A = Every.count() >> Prescaler2::BitShifts[TCCR2B];
			State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK2 = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 1 << WGM21;
			TIMSK2 = 1 << OCIE2A;
		}
		TIFR2 = -1;
		State.RepeatLeft = RepeatTimes;
	}
	else
		DoneCallback();
}
void TimerClass0::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	if (NumHalfPeriods)
	{
		AfterB += AfterA;
		TCNT0 = 0;
		if (const uint32_t OverflowTargetB = PrescalerOverflow<Prescaler01>(AfterB.count() >> 8, TCCR0B))
		{
			const uint8_t CandidateOCRA = AfterA.count() >> Prescaler01::MaxShift; // 可能会被自动截断后8位
			const uint8_t CandidateOCRB = AfterB.count() >> Prescaler01::MaxShift;
			// 由于0号计时器的OVF不可用，只能OCRA用于CTC，OCRB用于溢出计数，与AB任务无关
			if (const uint32_t OverflowTargetA = AfterA.count() >> Prescaler01::MaxShift + 8)
			{
				State.OverflowCount = OverflowTargetA;
				State.COMPB = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
						std::swap(State.COMPB, State.CandidateOVF);
						State.OverflowCount = OverflowTargetB;
					}
				};
				State.COMPA = [this, CandidateOCRB, DoA, DoneCallback]()
				{
					if (--State.RepeatLeft)
					{
						TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
						std::swap(State.COMPA, State.CandidateCOMP);
						OCR0A = CandidateOCRB;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoA();
					if (!State.RepeatLeft)
						DoneCallback();
				};
				State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
						std::swap(State.COMPB, State.CandidateOVF);
						State.OverflowCount = OverflowTargetA;
						TCCR0A = 1 << WGM01;
					}
				};
				State.CandidateCOMP = [this, CandidateOCRA, DoB, DoneCallback]()
				{
					if (--State.RepeatLeft)
					{
						TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
						std::swap(State.COMPA, State.CandidateCOMP);
						OCR0A = CandidateOCRA;
						TCCR0A = 0;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoB();
					if (!State.RepeatLeft)
						DoneCallback();
				};
			}
			else
			{
				State.COMPA = [this, CandidateOCRB, DoA, DoneCallback]()
				{
					if (--State.RepeatLeft)
					{
						TIMSK0 = (1 << TOIE0) | (1 << OCIE0B);
						std::swap(State.COMPA, State.CandidateCOMP);
						OCR0A = CandidateOCRB;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoA();
					if (!State.RepeatLeft)
						DoneCallback();
				};
				State.OverflowCount = OverflowTargetB;
				State.COMPB = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK0 = (1 << TOIE0) | (1 << OCIE0A) | (1 << OCIE0B);
						TCCR0A = 1 << WGM01;
						State.OverflowCount = OverflowTargetB;
					}
				};
				State.CandidateCOMP = [this, CandidateOCRA, DoB, DoneCallback]()
				{
					if (--State.RepeatLeft)
					{
						OCR0A = CandidateOCRA;
						TCCR0A = 0;
						std::swap(State.COMPA, State.CandidateCOMP);
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoB();
					if (!State.RepeatLeft)
						DoneCallback();
				};
			}
			OCR0B = 0;
			OCR0A = CandidateOCRA;
			TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
			TCCR0A = 0;
		}
		else
		{
			const uint8_t BitShift = Prescaler01::BitShifts[TCCR0B];
			OCR0B = AfterA.count() >> BitShift;
			State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				DoA();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			OCR0A = AfterB.count() >> BitShift;
			State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				DoB();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 1 << WGM01;
			TIMSK0 = (1 << TOIE0) | (1 << OCIE0A) | (1 << OCIE0B);
		}
		State.RepeatLeft = NumHalfPeriods;
		TIFR0 = -1;
	}
	else
		DoneCallback();
}
void TimerClass1::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	if (NumHalfPeriods)
	{
		AfterB += AfterA;
		TCNT = 0;
		if (const uint32_t OverflowTargetB = PrescalerOverflow<Prescaler01>(AfterB.count() >> 16, TCCRB))
		{
			if (const uint32_t OverflowTargetA = AfterA.count() >> Prescaler01::MaxShift + 16)
			{
				State.OverflowCount = OverflowTargetA;
				TIMSK = 1 << TOIE1;
				State.OVF = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK = (1 << OCIE1B) | (1 << TOIE1);
						std::swap(State.OVF, State.CandidateOVF);
						State.OverflowCount = OverflowTargetB;
					}
				};
				State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK = (1 << OCIE1A) | (1 << TOIE1);
						std::swap(State.OVF, State.CandidateOVF);
						State.OverflowCount = OverflowTargetA;
						TCCRB = 1 << WGM12 | Prescaler01::MaxClock;
					}
				};
			}
			else
			{
				State.OverflowCount = OverflowTargetB;
				TIMSK = (1 << TOIE1) | (1 << OCIE1B);
				State.OVF = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK = (1 << TOIE1) | (1 << OCIE1A);
						TCCRB = 1 << WGM12 | Prescaler01::MaxClock;
					}
				};
			}
			State.COMPB = [this, DoA, DoneCallback]()
			{
				TIMSK = --State.RepeatLeft ? 1 << TOIE1 : 0;
				DoA();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			State.COMPA = [this, DoB, DoneCallback]()
			{
				if (--State.RepeatLeft)
				{
					TIMSK = 1 << TOIE1;
					TCCRB = Prescaler01::MaxClock;
				}
				else
					TIMSK = 0;
				DoB();
				if (!State.RepeatLeft)
					DoneCallback();
			};
		}
		else
		{
			State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK = 0;
				DoA();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK = 0;
				DoB();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCRB |= 1 << WGM12;
			TIMSK = (1 << OCIE1A) | (1 << OCIE1B);
		}
		const uint8_t BitShift = Prescaler01::BitShifts[TCCRB];
		OCRA = AfterA.count() >> BitShift;
		OCRB = AfterB.count() >> BitShift;
		State.RepeatLeft = NumHalfPeriods;
		TCCRA = 0;
		TIFR = -1;
	}
	else
		DoneCallback();
}
void TimerClass2::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	if (NumHalfPeriods)
	{
		AfterB += AfterA;
		TCNT2 = 0;
		if (const uint32_t OverflowTargetB = PrescalerOverflow<Prescaler2>(AfterB.count() >> 8, TCCR2B))
		{
			if (const uint32_t OverflowTargetA = AfterA.count() >> Prescaler2::MaxShift + 8)
			{
				State.OverflowCount = OverflowTargetA;
				TIMSK2 = 1 << TOIE2;
				State.OVF = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK2 = (1 << OCIE2B) | (1 << TOIE2);
						std::swap(State.OVF, State.CandidateOVF);
						State.OverflowCount = OverflowTargetB;
					}
				};
				State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK2 = (1 << OCIE2A) | (1 << TOIE2);
						std::swap(State.OVF, State.CandidateOVF);
						State.OverflowCount = OverflowTargetA;
						TCCR2A = 1 << WGM21;
					}
				};
			}
			else
			{
				State.OverflowCount = OverflowTargetB;
				TIMSK2 = (1 << TOIE2) | (1 << OCIE2B);
				State.OVF = [this, OverflowTargetB]()
				{
					if (!--State.OverflowCount)
					{
						TIMSK2 = (1 << TOIE2) | (1 << OCIE2A);
						TCCR2A = 1 << WGM21;
					}
				};
			}
			State.COMPB = [this, DoA, DoneCallback]()
			{
				TIMSK2 = --State.RepeatLeft ? 1 << TOIE2 : 0;
				DoA();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			State.COMPA = [this, DoB, DoneCallback]()
			{
				if (--State.RepeatLeft)
				{
					TIMSK2 = 1 << TOIE2;
					TCCR2A = 0;
				}
				else
					TIMSK2 = 0;
				DoB();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 0;
		}
		else
		{
			State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK2 = 0;
				DoA();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--State.RepeatLeft)
					TIMSK2 = 0;
				DoB();
				if (!State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 1 << WGM21;
			TIMSK2 = (1 << OCIE1A) | (1 << OCIE1B);
		}
		const uint8_t BitShift = Prescaler2::BitShifts[TCCR2B];
		OCR2A = AfterA.count() >> BitShift;
		OCR2B = AfterB.count() >> BitShift;
		State.RepeatLeft = NumHalfPeriods;
		TIFR2 = -1;
	}
	else
		DoneCallback();
}
#endif