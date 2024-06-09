#include "Timers_one_for_all_2.hpp"
using namespace Timers_one_for_all;
const TimerClass *Timers_one_for_all::AllocateIdleTimer()
{
	for (int8_t T = NumTimers - 1; T >= 0; --T)
		if (!HardwareTimers[T]->Busy())
			return HardwareTimers[T];
	return nullptr;
}
// 需要测试：比较寄存器置0能否实现溢出中断？而不会一开始就中断？
#ifdef ARDUINO_ARCH_AVR
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
_TimerState Timers_one_for_all::_TimerStates[NumTimers];
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
	TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
	::StartTiming<Prescaler01>((TimerClass *)this);
	OCR0A = 0;
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
	TCNT0 = 0;
	volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler01>(Time.count() >> 8, TCCR0B) + 1;
	OCR0A = Time.count() >> Prescaler01::BitShifts[TCCR0B];
	State.COMPA = [&OverflowCount]()
	{
		if (!--OverflowCount)
			TIMSK0 = 1 << TOIE0; // 由中断负责停止计时器，避免OverflowCount下溢
	};
	TCCR0A = 0;
	TIFR0 = -1;
	TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
	while (OverflowCount)
		;
}
void TimerClass1::Delay(Tick Time) const
{
	TCNT = 0;
	volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler01>(Time.count() >> 16, TCCRB) + 1;
	OCRA = Time.count() >> Prescaler01::BitShifts[TCCRB];
	State.COMPA = [this, &OverflowCount]()
	{
		if (!--OverflowCount)
			TIMSK = 1 << TOIE1; // 由中断负责停止计时器，避免OverflowCount下溢
	};
	TCCRA = 0;
	TIFR = -1;
	TIMSK = 1 << OCIE1A;
	while (OverflowCount)
		;
}
void TimerClass2::Delay(Tick Time) const
{
	TCNT2 = 0;
	volatile uint32_t OverflowCount = PrescalerOverflow<Prescaler2>(Time.count() >> 8, TCCR2B) + 1;
	OCR2A = Time.count() >> Prescaler2::BitShifts[TCCR2B];
	State.COMPA = [&OverflowCount]()
	{
		if (!--OverflowCount)
			TIMSK2 = 1 << TOIE2; // 由中断负责停止计时器，避免OverflowCount下溢
	};
	TCCR2A = 0;
	TIFR2 = -1;
	TIMSK2 = 1 << OCIE2A;
	while (OverflowCount)
		;
}
void TimerClass0::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCNT0 = 0;
		State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 8, TCCR0B) + 1;
		OCR0A = After.count() >> Prescaler01::BitShifts[TCCR0B];
		State.COMPA = [this, Do]()
		{
			if (!--State.OverflowCount)
			{
				TIMSK0 = 1 << TOIE0;
				Do();
			}
		};
		TCCR0A = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
	}
	else
		Do();
}
void TimerClass1::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCNT = 0;
		State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 16, TCCRB) + 1;
		OCRA = After.count() >> Prescaler01::BitShifts[TCCRB];
		State.COMPA = [this, Do]()
		{
			if (!--State.OverflowCount)
			{
				TIMSK = 0;
				Do();
			}
		};
		TCCRA = 0;
		TIFR = -1;
		TIMSK = 1 << OCIE1A;
	}
	else
		Do();
}
void TimerClass2::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		TCNT2 = 0;
		State.OverflowCount = PrescalerOverflow<Prescaler2>(After.count() >> 8, TCCR2B) + 1;
		OCR2A = After.count() >> Prescaler2::BitShifts[TCCR2B];
		State.COMPA = [this, Do]()
		{
			if (!--State.OverflowCount)
			{
				TIMSK2 = 0;
				Do();
			}
		};
		TCCR2A = 0;
		TIFR2 = -1;
		TIMSK2 = 1 << OCIE2A;
	}
	else
		Do();
}
void TimerClass0::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		TCNT0 = 0;
		// 考虑到CTC需要提前设置，不能将溢出或非溢出情况混为一谈
		if (const uint32_t OverflowTarget = PrescalerOverflow<Prescaler01>(Every.count() >> 8, TCCR0B))
		{
			OCR0A = (Every.count() >> Prescaler01::MaxShift);
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
			OCR0B = 0;
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
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
static struct : public _TimerState
{
	uint32_t LOAD;
	std::function<void()> OverflowHandlerA;
	std::function<void()> DoHandlerA;
	std::function<void()> OverflowHandlerB;
	std::function<void()> DoHandlerB;
	uint32_t OverflowCount;
} SystemState;
int sysTickHook()
{
	if (SystemState.Handler)
		SystemState.Handler();
	return 0;
}
bool SystemTimerClass::Busy() const
{
	return (bool)SystemState.Handler;
}
void SystemTimerClass::Pause() const
{
	if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
	{
		SystemState.LOAD = SysTick->LOAD;
		SysTick->LOAD = SysTick->VAL;
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}
}
void SystemTimerClass::Continue() const
{
	if (!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk))
	{
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
		SysTick->LOAD = SystemState.LOAD;
	}
}
void SystemTimerClass::Stop() const
{
	SystemState.Handler = nullptr;
}
constexpr uint32_t SysTick_CTRL_MCK8 = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
constexpr uint32_t SysTick_CTRL_MCK = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_MCK8;
void SystemTimerClass::StartTiming() const
{
	SysTick->LOAD = -1;
	SysTick->CTRL = SysTick_CTRL_MCK;
	SystemState.OverflowCount = 8;
	SystemState.Handler = []()
	{
		if (!--SystemState.OverflowCount)
		{
			SysTick->CTRL = SysTick_CTRL_MCK8;
			SystemState.OverflowCount = 1;
			SystemState.Handler = []()
			{ SystemState.OverflowCount++; };
		}
	};
}
Tick SystemTimerClass::GetTiming() const
{
	return Tick(((uint64_t)(SystemState.OverflowCount + 1) << 24) - SysTick->VAL << (SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk ? 0 : 3));
}
void SystemTimerClass::Delay(Tick Time) const
{
	uint64_t TimeTicks = Time.count();
	uint32_t CTRL = SysTick_CTRL_MCK;
	volatile uint32_t OverflowCount;
	if (TimeTicks >> 24)
	{
		TimeTicks >>= 3;
		if (OverflowCount = TimeTicks >> 24)
		{
			SysTick->LOAD = -1;
			SysTick->CTRL = SysTick_CTRL_MCK8;
			SystemState.Handler = [&OverflowCount]()
			{ --OverflowCount; };
			while (OverflowCount)
				;
		}
		CTRL = SysTick_CTRL_MCK8;
	}
	OverflowCount = 1;
	SysTick->LOAD = TimeTicks & 0xffffff;
	SysTick->CTRL = CTRL;
	SystemState.Handler = [&OverflowCount]()
	{ OverflowCount = 0; };
	while (OverflowCount)
		;
}
void SystemTimerClass::DoAfter(Tick After, std::function<void()> Do) const
{
	if (uint64_t TimeTicks = After.count())
	{
		uint32_t CTRL = SysTick_CTRL_MCK;
		Do = [Do]()
		{
			SystemState.Handler = nullptr;
			Do();
		};
		if (TimeTicks >> 24)
		{
			TimeTicks >>= 3;
			if (SystemState.OverflowCount = TimeTicks >> 24)
			{
				SysTick->LOAD = -1;
				SysTick->CTRL = SysTick_CTRL_MCK8;
				SystemState.Handler = [TimeTicks, Do]()
				{
					if (!--SystemState.OverflowCount)
					{
						SysTick->LOAD = TimeTicks & 0xffffff;
						SysTick->CTRL = SysTick_CTRL_MCK8;
						SystemState.Handler = Do;
					}
				};
				return;
			}
			CTRL = SysTick_CTRL_MCK8;
		}
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
		SystemState.Handler = Do;
	}
	else
		Do();
}
void SystemTimerClass::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		uint64_t TimeTicks = Every.count();
		uint32_t CTRL = SysTick_CTRL_MCK;
		SystemState.RepeatLeft = RepeatTimes;
		if (TimeTicks >> 24)
		{
			TimeTicks >>= 3;
			if (const uint32_t OverflowTarget = TimeTicks >> 24)
			{
				SysTick->LOAD = -1;
				SysTick->CTRL = SysTick_CTRL_MCK8;
				const uint32_t TicksLeft = TimeTicks & 0xffffff;
				SystemState.OverflowCount = OverflowTarget;
				SystemState.OverflowHandlerA = [TicksLeft]()
				{
					if (!--SystemState.OverflowCount)
					{
						SysTick->LOAD = TicksLeft;
						SysTick->CTRL = SysTick_CTRL_MCK8;
						SystemState.Handler = SystemState.DoHandlerA;
					}
				};
				SystemState.DoHandlerA = [Do, DoneCallback, OverflowTarget]()
				{
					if (--SystemState.RepeatLeft)
					{
						SysTick->LOAD = -1;
						SysTick->CTRL = SysTick_CTRL_MCK8;
						SystemState.OverflowCount = OverflowTarget;
						SystemState.Handler = SystemState.OverflowHandlerA;
					}
					else
						SystemState.Handler = nullptr;
					Do();
					if (!SystemState.RepeatLeft)
						DoneCallback();
				};
				return;
			}
			CTRL = SysTick_CTRL_MCK8;
		}
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
		SystemState.Handler = [Do, DoneCallback]()
		{
			if (!--SystemState.RepeatLeft)
				SystemState.Handler = nullptr;
			Do();
			if (!SystemState.RepeatLeft)
				DoneCallback();
		};
	}
	else
		DoneCallback();
}
void SystemTimerClass::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	if (NumHalfPeriods)
	{
		uint64_t TimeTicksA = AfterA.count();
		uint64_t TimeTicksB = AfterB.count();
		uint32_t CTRL = SysTick_CTRL_MCK;
		if (max(TimeTicksA, TimeTicksB) >> 24)
		{
			TimeTicksA >>= 3;
			TimeTicksB >>= 3;
			CTRL = SysTick_CTRL_MCK8;
		}
		const uint32_t OverflowTargetA = TimeTicksA >> 24;
		if (OverflowTargetA)
		{
			SysTick->LOAD = -1;
			SysTick->CTRL = SysTick_CTRL_MCK8;
			SystemState.OverflowCount = OverflowTargetA;
			const uint32_t TicksLeft = TimeTicksA & 0xffffff;
			SystemState.OverflowHandlerA = [TicksLeft]()
			{
				if (!--SystemState.OverflowCount)
				{
					SysTick->LOAD = TicksLeft;
					SysTick->CTRL = SysTick_CTRL_MCK8;
					SystemState.Handler = SystemState.DoHandlerA;
				}
			};
			SystemState.DoHandlerB = [DoB, DoneCallback, OverflowTargetA, TicksLeft]()
			{
				if (--SystemState.RepeatLeft)
				{
					SysTick->LOAD = -1;
					SysTick->CTRL = SysTick_CTRL_MCK8;
					SystemState.Handler = SystemState.OverflowHandlerA;
					SystemState.OverflowCount = OverflowTargetA;
				}
				else
					SystemState.Handler = nullptr;
				DoB();
				if (SystemState.RepeatLeft)
					DoneCallback();
			};
		}
		else
		{
			const uint32_t TicksLeft = TimeTicksA;
			SysTick->LOAD = TicksLeft;
			SysTick->CTRL = CTRL;
			SystemState.DoHandlerB = [DoB, DoneCallback, TicksLeft]()
			{
				if (--SystemState.RepeatLeft)
				{
					SysTick->LOAD = TicksLeft;
					SysTick->CTRL = SysTick->CTRL;
					SystemState.Handler = SystemState.DoHandlerA;
				}
				else
					SystemState.Handler = nullptr;
				DoB();
				if (SystemState.RepeatLeft)
					DoneCallback();
			};
		}
		if (const uint32_t OverflowTargetB = TimeTicksB >> 24)
		{
			const uint32_t TicksLeft = TimeTicksB & 0xffffff;
			SystemState.OverflowHandlerB = [TicksLeft]()
			{
				if (!--SystemState.OverflowCount)
				{
					SysTick->LOAD = TicksLeft;
					SysTick->CTRL = SysTick_CTRL_MCK8;
					SystemState.Handler = SystemState.DoHandlerB;
				}
			};
			SystemState.DoHandlerA = [DoA, DoneCallback, OverflowTargetB, TicksLeft]()
			{
				if (--SystemState.RepeatLeft)
				{
					SysTick->LOAD = -1;
					SysTick->CTRL = SysTick_CTRL_MCK8;
					SystemState.Handler = SystemState.OverflowHandlerB;
					SystemState.OverflowCount = OverflowTargetB;
				}
				else
					SystemState.Handler = nullptr;
				DoA();
				if (SystemState.RepeatLeft)
					DoneCallback();
			};
		}
		else
		{
			const uint32_t TicksLeft = TimeTicksB;
			SystemState.DoHandlerA = [DoA, DoneCallback, TicksLeft]()
			{
				if (--SystemState.RepeatLeft)
				{
					SysTick->LOAD = TicksLeft;
					SysTick->CTRL = SysTick->CTRL;
					SystemState.Handler = SystemState.DoHandlerB;
				}
				else
					SystemState.Handler = nullptr;
				DoA();
				if (SystemState.RepeatLeft)
					DoneCallback();
			};
		}
		SystemState.Handler = OverflowTargetA ? SystemState.OverflowHandlerA : SystemState.DoHandlerA;
		SystemState.RepeatLeft = NumHalfPeriods;
	}
	else
		DoneCallback();
}
#endif
#ifdef TOFA_REALTIMER
static struct : public _TimerState
{
	uint16_t OverflowCount;
	std::function<void()> CandidateHandler;
} RealState;
void RTT_Handler()
{
	if (RealState.Handler)
		RealState.Handler();
}
bool RealTimerClass::Busy() const
{
	return (bool)RealState.Handler;
}
void RealTimerClass::Pause() const
{
	// 伪暂停算法
	if (RTT->RTT_MR & RTT_MR_ALMIEN)
	{
		RTT->RTT_AR -= RTT->RTT_VR;
		RTT->RTT_MR &= ~RTT_MR_ALMIEN;
	}
};
void RealTimerClass::Continue() const
{
	if (!(RTT->RTT_MR & RTT_MR_ALMIEN) && RealState.Handler)
		RTT->RTT_MR |= RTT_MR_ALMIEN | RTT_MR_RTTRST;
	// 可能存在问题：RTTRST可能导致AR变成-1
}
void RealTimerClass::Stop() const
{
	RealState.Handler = nullptr;
	RTT->RTT_MR = 0;
}
void RealTimerClass::StartTiming() const
{
	RTT->RTT_MR = 1 | RTT_MR_ALMIEN | RTT_MR_RTTRST;
	RTT->RTT_AR = -1;
	RealState.OverflowCount = 0;
	RealState.Handler = []()
	{
		if (++RealState.OverflowCount == 2)
		{
			const uint16_t RTPRES = RTT->RTT_MR << 1;
			RTT->RTT_MR = RTPRES | RTT_MR_ALMIEN;
			// 可能存在问题：MR的修改可能在RTTRST之前不会生效
			RealState.OverflowCount = 1;
			if (!RTPRES)
				RealState.Handler = []()
				{ RealState.OverflowCount++; };
		}
		// 可能存在问题：中断寄存器尚未清空导致中断再次触发
	};
	NVIC_EnableIRQ(RTT_IRQn);
}
using RealTick = std::chrono::duration<uint64_t, std::ratio<1, 32768>>;
Tick RealTimerClass::GetTiming() const
{
	uint64_t TimerTicks = (RealState.OverflowCount + 1) << 32 + RTT->RTT_VR - RTT->RTT_AR;
	if (const uint16_t RTPRES = RTT->RTT_MR)
		TimerTicks *= RTPRES;
	else
		TimerTicks <<= 16;
	return std::chrono::duration_cast<Tick>(RealTick(TimerTicks));
}
void RealTimerClass::Delay(Tick Time) const
{
	const uint64_t TimerTicks = std::chrono::duration_cast<RealTick>(Time).count();
	volatile uint16_t OverflowCount = 1;
	if (const uint32_t RTPRES = TimerTicks >> 32)
	{
		if ((OverflowCount += RTPRES >> 16) > 1)
		{
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
			RTT->RTT_AR = TimerTicks >> 16;
		}
		else
		{
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
			RTT->RTT_AR = -1;
		}
	}
	else
	{
		RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
		RTT->RTT_AR = TimerTicks;
	}
	RealState.Handler = [&OverflowCount]()
	{
		if (!--OverflowCount)
		{
			RealState.Handler = nullptr;
			RTT->RTT_MR = 0;
		}
	};
	NVIC_EnableIRQ(RTT_IRQn);
	while (OverflowCount)
		;
}
void RealTimerClass::DoAfter(Tick After, std::function<void()> Do) const
{
	if (After.count())
	{
		Do = [Do]()
		{
			RealState.Handler = nullptr;
			RTT->RTT_MR = 0;
			Do();
		};
		const uint64_t TimerTicks = std::chrono::duration_cast<RealTick>(After).count();
		NVIC_EnableIRQ(RTT_IRQn);
		if (const uint32_t RTPRES = TimerTicks >> 32)
		{
			if (RealState.OverflowCount = RTPRES >> 16)
			{
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				RTT->RTT_AR = TimerTicks >> 16;
				RealState.Handler = [Do]()
				{
					if (!--RealState.OverflowCount)
						Do();
				};
				return;
			}
			else
			{
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
				RTT->RTT_AR = -1;
			}
		}
		else
		{
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
			RTT->RTT_AR = TimerTicks;
		}
		RealState.Handler = Do;
	}
	else
		Do();
}
void RealTimerClass::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		const uint64_t TimerTicks = std::chrono::duration_cast<RealTick>(Every).count();
		if (const uint32_t RTPRES = TimerTicks >> 32)
		{
			if (uint16_t OverflowTarget = RTPRES >> 16)
			{
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				const uint32_t RTT_AR = TimerTicks >> 16;
				RTT->RTT_AR = RTT_AR;
				RealState.OverflowCount = ++OverflowTarget;
				RealState.Handler = [Do, RTT_AR, OverflowTarget, DoneCallback]()
				{
					if (!--RealState.OverflowCount)
					{
						if (--RealState.RepeatLeft)
						{
							RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
							RTT->RTT_AR = RTT_AR; // 伪暂停算法可能修改AR，所以每次都要重新设置
							RealState.OverflowCount = OverflowTarget;
						}
						else
						{
							RTT->RTT_MR = 0;
							RealState.Handler = nullptr;
						}
						Do();
						if (!RealState.RepeatLeft)
							DoneCallback();
					}
				};
			}
			else
			{
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
				RTT->RTT_AR = -1;
				RealState.Handler = [Do, DoneCallback]()
				{
					if (!--RealState.RepeatLeft)
					{
						RTT->RTT_MR = 0;
						RealState.Handler = nullptr;
					}
					Do();
					if (!RealState.RepeatLeft)
						DoneCallback();
				};
			}
		}
		else
		{
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
			const uint32_t RTT_AR = TimerTicks;
			RTT->RTT_AR = RTT_AR;
			RealState.Handler = [Do, DoneCallback, RTT_AR]()
			{
				if (--RealState.RepeatLeft)
					RTT->RTT_AR += RTT_AR;
				else
				{
					RTT->RTT_MR = 0;
					RealState.Handler = nullptr;
				}
				Do();
				if (!RealState.RepeatLeft)
					DoneCallback();
			};
		}
		NVIC_EnableIRQ(RTT_IRQn);
	}
	else
		DoneCallback();
}
void RealTimerClass::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	if (NumHalfPeriods)
	{
		const uint64_t TimerTicksA = std::chrono::duration_cast<RealTick>(AfterA).count();
		const uint64_t TimerTicksB = std::chrono::duration_cast<RealTick>(AfterB).count();
		const uint64_t MaxTicks = max(TimerTicksA, TimerTicksB);
		RealState.RepeatLeft = NumHalfPeriods;
		NVIC_EnableIRQ(RTT_IRQn);
		uint32_t RTPRES = MaxTicks >> 32;
		uint32_t RTT_AR_A;
		uint32_t RTT_AR_B;
		if (RTPRES)
		{
			if (RTPRES >> 16)
			{
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				RTT_AR_A = TimerTicksA >> 16;
				RTT->RTT_AR = RTT_AR_A;
				const uint16_t OverflowTargetA = TimerTicksA >> 48 + 1;
				RealState.OverflowCount = OverflowTargetA;
				const uint16_t OverflowTargetB = TimerTicksB >> 48 + 1;
				RTT_AR_B = TimerTicksB >> 16;
				RealState.Handler = [OverflowTargetB, RTT_AR_B, DoA, DoneCallback]()
				{
					if (!--RealState.OverflowCount)
					{
						if (--RealState.RepeatLeft)
						{
							RealState.OverflowCount += OverflowTargetB;
							RTT->RTT_AR += RTT_AR_B;
							std::swap(RealState.Handler, RealState.CandidateHandler);
						}
						else
						{
							RTT->RTT_MR = 0;
							RealState.Handler = nullptr;
						}
						DoA();
						if (!RealState.RepeatLeft)
							DoneCallback();
					}
				};
				RealState.CandidateHandler = [OverflowTargetA, RTT_AR_A, DoB, DoneCallback]()
				{
					if (!--RealState.OverflowCount)
					{
						if (--RealState.RepeatLeft)
						{
							RealState.OverflowCount += OverflowTargetA;
							RTT->RTT_AR += RTT_AR_A;
							std::swap(RealState.Handler, RealState.CandidateHandler);
						}
						else
						{
							RTT->RTT_MR = 0;
							RealState.Handler = nullptr;
						}
						DoB();
						if (!RealState.RepeatLeft)
							DoneCallback();
					}
				};
				return;
			}
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
			RTT_AR_A = TimerTicksA / RTPRES;
			RTT_AR_B = TimerTicksB / RTPRES;
		}
		else
		{
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
			RTT_AR_A = TimerTicksA;
			RTT_AR_B = TimerTicksB;
		}
		RTT->RTT_AR = RTT_AR_A;
		RealState.Handler = [RTT_AR_B, DoA, DoneCallback]()
		{
			if (--RealState.RepeatLeft)
			{
				RTT->RTT_AR += RTT_AR_B;
				std::swap(RealState.Handler, RealState.CandidateHandler);
			}
			else
			{
				RTT->RTT_MR = 0;
				RealState.Handler = nullptr;
			}
			DoA();
			if (!RealState.RepeatLeft)
				DoneCallback();
		};
		RealState.CandidateHandler = [RTT_AR_A, DoB, DoneCallback]()
		{
			if (--RealState.RepeatLeft)
			{
				RTT->RTT_AR += RTT_AR_A;
				std::swap(RealState.Handler, RealState.CandidateHandler);
			}
			else
			{
				RTT->RTT_MR = 0;
				RealState.Handler = nullptr;
			}
			DoB();
			if (!RealState.RepeatLeft)
				DoneCallback();
		};
	}
	else
		DoneCallback();
}
#endif
_PeripheralState Timers_one_for_all::_TimerStates[(uint8_t)_PeripheralEnum::NumPeripherals];

#endif