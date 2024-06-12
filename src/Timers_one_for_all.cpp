#include <Timers_one_for_all.hpp>
#include <iostream>
using namespace Timers_one_for_all;
const TimerClass *Timers_one_for_all::AllocateTimer()
{
	for (int8_t T = NumTimers - 1; T >= 0; --T)
	{
		const TimerClass *const HT = HardwareTimers[T];
		if (HT->Allocatable())
		{
			HT->Allocatable(false);
			return HT;
		}
	}
	return nullptr;
}
#ifdef ARDUINO_ARCH_AVR
// 需要测试：比较寄存器置0能否实现溢出中断？而不会一开始就中断？
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
	using type = typename U8SequencePrepend<Second - First, typename U8SequenceDiff<U8Sequence<Second, Rest...>>::type>::type;
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
	using type = typename U8SequencePrepend<PrescalerIndex, typename BitLimit<T, BitIndex + 1, PrescalerIndex>::type>::type;
};
template <uint8_t BitIndex, uint8_t PrescalerIndex, uint8_t... Rest>
struct BitLimit<U8Sequence<BitIndex + 1, Rest...>, BitIndex, PrescalerIndex>
{
	using type = typename U8SequencePrepend<PrescalerIndex, typename BitLimit<U8Sequence<Rest...>, BitIndex + 1, PrescalerIndex + 1>::type>::type;
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
struct PrescalerType : public PrescalerDiff<typename U8SequenceDiff<U8Sequence<0, BitShift...>>::type>, public BitsToPrescaler_s<typename BitLimit<U8Sequence<BitShift...>>::type>
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
#define TimerIsr(T)                                        \
	ISR(TIMER##T##_OVF_vect)                               \
	{                                                      \
		_TimerStates[(size_t)TimerEnum::Timer##T].OVF();   \
	}                                                      \
	ISR(TIMER##T##_COMPA_vect)                             \
	{                                                      \
		_TimerStates[(size_t)TimerEnum::Timer##T].COMPA(); \
	}                                                      \
	ISR(TIMER##T##_COMPB_vect)                             \
	{                                                      \
		_TimerStates[(size_t)TimerEnum::Timer##T].COMPB(); \
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
void TimerClass0::StartTiming() const
{
	TCNT0 = 0;
	TCCR0B = 1;
	TCCR0A = 0;
	TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
	_State.OverflowCount = 0;
	_State.COMPA = [this]()
	{
		if (++_State.OverflowCount == Prescaler01::AdvanceFactor[TCCR0B])
		{
			_State.OverflowCount = 1;
			if ((++TCCR0B >= std::extent_v<decltype(Prescaler01::AdvanceFactor)>))
				_State.COMPA = [this]()
				{ _State.OverflowCount++; };
		}
	};
	OCR0A = 0;
	TIFR0 = -1;
}
void TimerClass1::StartTiming() const
{
	TCNT = 0;
	TCCRB = 1;
	TCCRA = 0;
	TIMSK = 1 << TOIE1;
	_State.OverflowCount = 0;
	_State.OVF = [this]()
	{
		if (++_State.OverflowCount == Prescaler01::AdvanceFactor[TCCRB])
		{
			_State.OverflowCount = 1;
			if ((++TCCRB >= std::extent_v<decltype(Prescaler01::AdvanceFactor)>))
				_State.OVF = [this]()
				{ _State.OverflowCount++; };
		}
	};
	TIFR = -1;
}
void TimerClass2::StartTiming() const
{
	TCNT2 = 0;
	TCCR2B = 1;
	TCCR2A = 0;
	TIMSK2 = 1 << TOIE1;
	_State.OverflowCount = 0;
	_State.OVF = [this]()
	{
		if (++_State.OverflowCount == Prescaler01::AdvanceFactor[TCCR2B])
		{
			_State.OverflowCount = 1;
			if ((++TCCR2B >= std::extent_v<decltype(Prescaler01::AdvanceFactor)>))
				_State.OVF = [this]()
				{ _State.OverflowCount++; };
		}
	};
	TIFR2 = -1;
}
Tick TimerClass0::GetTiming() const
{
	const uint8_t Clock = TCCR0B;
	return Tick(((uint64_t)_State.OverflowCount << 8) + TCNT0 << Prescaler01::BitShifts[Clock ? Clock : _State.Clock]);
}
Tick TimerClass1::GetTiming() const
{
	const uint8_t Clock = TCCRB;
	return Tick(((uint64_t)_State.OverflowCount << 16) + TCNT << Prescaler01::BitShifts[Clock ? Clock : _State.Clock]);
}
Tick TimerClass2::GetTiming() const
{
	const uint8_t Clock = TCCR2B;
	return Tick(((uint64_t)_State.OverflowCount << 8) + TCNT2 << Prescaler2::BitShifts[Clock ? Clock : _State.Clock]);
}
// 返回OverflowTarget
template <typename Prescaler>
uint32_t PrescalerOverflow(uint32_t Cycles, volatile uint8_t &Clock)
{
	constexpr uint8_t MaxBits = std::extent_v<decltype(Prescaler::BitsToPrescaler)>;
	const uint8_t CycleBits = Cycles ? sizeof(Cycles) * 8 - 1 - __builtin_clz(Cycles) : 0; // Cycles为0时避免CycleBits下溢
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
	_State.COMPA = [&OverflowCount]()
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
	_State.COMPA = [this, &OverflowCount]()
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
	_State.COMPA = [&OverflowCount]()
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
	TCNT0 = 0;
	_State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 8, TCCR0B) + 1;
	OCR0A = After.count() >> Prescaler01::BitShifts[TCCR0B];
	_State.COMPA = [this, Do]()
	{
		if (!--_State.OverflowCount)
		{
			TIMSK0 = 1 << TOIE0;
			Do();
		}
	};
	TCCR0A = 0;
	TIFR0 = -1;
	TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
}
void TimerClass1::DoAfter(Tick After, std::function<void()> Do) const
{
	TCNT = 0;
	_State.OverflowCount = PrescalerOverflow<Prescaler01>(After.count() >> 16, TCCRB) + 1;
	OCRA = After.count() >> Prescaler01::BitShifts[TCCRB];
	_State.COMPA = [this, Do]()
	{
		if (!--_State.OverflowCount)
		{
			TIMSK = 0;
			Do();
		}
	};
	TCCRA = 0;
	TIFR = -1;
	TIMSK = 1 << OCIE1A;
}
void TimerClass2::DoAfter(Tick After, std::function<void()> Do) const
{
	TCNT2 = 0;
	_State.OverflowCount = PrescalerOverflow<Prescaler2>(After.count() >> 8, TCCR2B) + 1;
	OCR2A = After.count() >> Prescaler2::BitShifts[TCCR2B];
	_State.COMPA = [this, Do]()
	{
		if (!--_State.OverflowCount)
		{
			TIMSK2 = 0;
			Do();
		}
	};
	TCCR2A = 0;
	TIFR2 = -1;
	TIMSK2 = 1 << OCIE2A;
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
			_State.OverflowCount = OverflowTarget;
			_State.COMPB = [this]()
			{
				if (!--_State.OverflowCount)
				{
					TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
					TCCR0A = 1 << WGM01;
				}
			};
			_State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCR0A = 0;
				if (--_State.RepeatLeft)
				{
					_State.OverflowCount = OverflowTarget;
					TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
				}
				else
					TIMSK0 = 1 << TOIE0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 0;
			TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
			OCR0B = 0;
		}
		else
		{
			OCR0A = Every.count() >> Prescaler01::BitShifts[TCCR0B];
			_State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 1 << WGM01;
			TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
		}
		TIFR0 = -1;
		_State.RepeatLeft = RepeatTimes;
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
			_State.OverflowCount = OverflowTarget;
			_State.OVF = [this]()
			{
				if (!--_State.OverflowCount)
				{
					TIMSK = 1 << OCIE1A;
					TCCRB = (1 << WGM12) | Prescaler01::MaxClock;
				}
			};
			_State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCRB = Prescaler01::MaxClock;
				if (--_State.RepeatLeft)
				{
					_State.OverflowCount = OverflowTarget;
					TIMSK = 1 << TOIE1;
				}
				else
					TIMSK = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TIMSK = 1 << TOIE1;
		}
		else
		{
			OCRA = Every.count() >> Prescaler01::BitShifts[TCCRB];
			_State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCRB |= 1 << WGM12;
			TIMSK = 1 << OCIE1A;
		}
		TCCRA = 0;
		TIFR = -1;
		_State.RepeatLeft = RepeatTimes;
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
			_State.OverflowCount = OverflowTarget;
			_State.OVF = [this]()
			{
				if (!--_State.OverflowCount)
				{
					TIMSK2 = 1 << OCIE2A;
					TCCR2A = 1 << WGM21;
				}
			};
			_State.COMPA = [this, Do, DoneCallback, OverflowTarget]()
			{
				TCCR2A = 0;
				if (--_State.RepeatLeft)
				{
					_State.OverflowCount = OverflowTarget;
					TIMSK2 = 1 << TOIE2;
				}
				else
					TIMSK2 = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 0;
			TIMSK2 = 1 << TOIE2;
		}
		else
		{
			OCR2A = Every.count() >> Prescaler2::BitShifts[TCCR2B];
			_State.COMPA = [this, Do, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK2 = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 1 << WGM21;
			TIMSK2 = 1 << OCIE2A;
		}
		TIFR2 = -1;
		_State.RepeatLeft = RepeatTimes;
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
				_State.OverflowCount = OverflowTargetA;
				_State.COMPB = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
						std::swap(_State.COMPB, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetB;
					}
				};
				_State.COMPA = [this, CandidateOCRB, DoA, DoneCallback]()
				{
					if (--_State.RepeatLeft)
					{
						TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
						std::swap(_State.COMPA, _State.CandidateCOMP);
						OCR0A = CandidateOCRB;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoA();
					if (!_State.RepeatLeft)
						DoneCallback();
				};
				_State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
						std::swap(_State.COMPB, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetA;
						TCCR0A = 1 << WGM01;
					}
				};
				_State.CandidateCOMP = [this, CandidateOCRA, DoB, DoneCallback]()
				{
					if (--_State.RepeatLeft)
					{
						TIMSK0 = (1 << OCIE0B) | (1 << TOIE0);
						std::swap(_State.COMPA, _State.CandidateCOMP);
						OCR0A = CandidateOCRA;
						TCCR0A = 0;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoB();
					if (!_State.RepeatLeft)
						DoneCallback();
				};
			}
			else
			{
				_State.COMPA = [this, CandidateOCRB, DoA, DoneCallback]()
				{
					if (--_State.RepeatLeft)
					{
						TIMSK0 = (1 << TOIE0) | (1 << OCIE0B);
						std::swap(_State.COMPA, _State.CandidateCOMP);
						OCR0A = CandidateOCRB;
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoA();
					if (!_State.RepeatLeft)
						DoneCallback();
				};
				_State.OverflowCount = OverflowTargetB;
				_State.COMPB = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK0 = (1 << TOIE0) | (1 << OCIE0A) | (1 << OCIE0B);
						TCCR0A = 1 << WGM01;
						_State.OverflowCount = OverflowTargetB;
					}
				};
				_State.CandidateCOMP = [this, CandidateOCRA, DoB, DoneCallback]()
				{
					if (--_State.RepeatLeft)
					{
						OCR0A = CandidateOCRA;
						TCCR0A = 0;
						std::swap(_State.COMPA, _State.CandidateCOMP);
					}
					else
						TIMSK0 = 1 << TOIE0;
					DoB();
					if (!_State.RepeatLeft)
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
			_State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			OCR0A = AfterB.count() >> BitShift;
			_State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR0A = 1 << WGM01;
			TIMSK0 = (1 << TOIE0) | (1 << OCIE0A) | (1 << OCIE0B);
		}
		_State.RepeatLeft = NumHalfPeriods;
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
				_State.OverflowCount = OverflowTargetA;
				TIMSK = 1 << TOIE1;
				_State.OVF = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK = (1 << OCIE1B) | (1 << TOIE1);
						std::swap(_State.OVF, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetB;
					}
				};
				_State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK = (1 << OCIE1A) | (1 << TOIE1);
						std::swap(_State.OVF, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetA;
						TCCRB = (1 << WGM12) | Prescaler01::MaxClock;
					}
				};
			}
			else
			{
				_State.OverflowCount = OverflowTargetB;
				TIMSK = (1 << TOIE1) | (1 << OCIE1B);
				_State.OVF = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK = (1 << TOIE1) | (1 << OCIE1A);
						TCCRB = (1 << WGM12) | Prescaler01::MaxClock;
					}
				};
			}
			_State.COMPB = [this, DoA, DoneCallback]()
			{
				TIMSK = --_State.RepeatLeft ? 1 << TOIE1 : 0;
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			_State.COMPA = [this, DoB, DoneCallback]()
			{
				if (--_State.RepeatLeft)
				{
					TIMSK = 1 << TOIE1;
					TCCRB = Prescaler01::MaxClock;
				}
				else
					TIMSK = 0;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
		}
		else
		{
			_State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK = 0;
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			_State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK = 0;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCRB |= 1 << WGM12;
			TIMSK = (1 << OCIE1A) | (1 << OCIE1B);
		}
		const uint8_t BitShift = Prescaler01::BitShifts[TCCRB];
		OCRA = AfterA.count() >> BitShift;
		OCRB = AfterB.count() >> BitShift;
		_State.RepeatLeft = NumHalfPeriods;
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
				_State.OverflowCount = OverflowTargetA;
				TIMSK2 = 1 << TOIE2;
				_State.OVF = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK2 = (1 << OCIE2B) | (1 << TOIE2);
						std::swap(_State.OVF, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetB;
					}
				};
				_State.CandidateOVF = [this, OverflowTargetA]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK2 = (1 << OCIE2A) | (1 << TOIE2);
						std::swap(_State.OVF, _State.CandidateOVF);
						_State.OverflowCount = OverflowTargetA;
						TCCR2A = 1 << WGM21;
					}
				};
			}
			else
			{
				_State.OverflowCount = OverflowTargetB;
				TIMSK2 = (1 << TOIE2) | (1 << OCIE2B);
				_State.OVF = [this, OverflowTargetB]()
				{
					if (!--_State.OverflowCount)
					{
						TIMSK2 = (1 << TOIE2) | (1 << OCIE2A);
						TCCR2A = 1 << WGM21;
					}
				};
			}
			_State.COMPB = [this, DoA, DoneCallback]()
			{
				TIMSK2 = --_State.RepeatLeft ? 1 << TOIE2 : 0;
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			_State.COMPA = [this, DoB, DoneCallback]()
			{
				if (--_State.RepeatLeft)
				{
					TIMSK2 = 1 << TOIE2;
					TCCR2A = 0;
				}
				else
					TIMSK2 = 0;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 0;
		}
		else
		{
			_State.COMPB = [this, DoA, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK2 = 0;
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			_State.COMPA = [this, DoB, DoneCallback]()
			{
				if (!--_State.RepeatLeft)
					TIMSK2 = 0;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			TCCR2A = 1 << WGM21;
			TIMSK2 = (1 << OCIE1A) | (1 << OCIE1B);
		}
		const uint8_t BitShift = Prescaler2::BitShifts[TCCR2B];
		OCR2A = AfterA.count() >> BitShift;
		OCR2B = AfterB.count() >> BitShift;
		_State.RepeatLeft = NumHalfPeriods;
		TIFR2 = -1;
	}
	else
		DoneCallback();
}
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
static struct SystemTimerState : public _TimerState
{
	uint32_t LOAD;
	std::function<void()> OverflowHandlerA;
	std::function<void()> DoHandlerA;
	std::function<void()> OverflowHandlerB;
	std::function<void()> DoHandlerB;
	uint32_t OverflowCount;
	SystemTimerState() { Handler = nullptr; }
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
	uint64_t TimeTicks = After.count();
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
bool SystemTimerClass::Allocatable() const
{
	return SystemState.Allocatable;
}
void SystemTimerClass::Allocatable(bool A) const
{
	SystemState.Allocatable = A;
}
#endif
#ifdef TOFA_REALTIMER
static struct RealTimerState : public _TimerState
{
	uint16_t OverflowCount;
	std::function<void()> CandidateHandler;
	RealTimerState() { Handler = nullptr; }
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
using SlowTick = std::chrono::duration<uint64_t, std::ratio<1, 32768>>;
Tick RealTimerClass::GetTiming() const
{
	uint64_t TimerTicks = ((uint64_t)(RealState.OverflowCount + 1) << 32) + RTT->RTT_VR - RTT->RTT_AR;
	if (const uint16_t RTPRES = RTT->RTT_MR)
		TimerTicks *= RTPRES;
	else
		TimerTicks <<= 16;
	return std::chrono::duration_cast<Tick>(SlowTick(TimerTicks));
}
void RealTimerClass::Delay(Tick Time) const
{
	const uint64_t TimerTicks = std::chrono::duration_cast<SlowTick>(Time).count();
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

	Do = [Do]()
	{
		RealState.Handler = nullptr;
		RTT->RTT_MR = 0;
		Do();
	};
	const uint64_t TimerTicks = std::chrono::duration_cast<SlowTick>(After).count();
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
void RealTimerClass::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	if (RepeatTimes)
	{
		const uint64_t TimerTicks = std::chrono::duration_cast<SlowTick>(Every).count();
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
		const uint64_t TimerTicksA = std::chrono::duration_cast<SlowTick>(AfterA).count();
		const uint64_t TimerTicksB = std::chrono::duration_cast<SlowTick>(AfterB).count();
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
				const uint16_t OverflowTargetA = (TimerTicksA >> 48) + 1;
				RealState.OverflowCount = OverflowTargetA;
				const uint16_t OverflowTargetB = (TimerTicksB >> 48) + 1;
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
bool RealTimerClass::Allocatable() const
{
	return RealState.Allocatable;
}
void RealTimerClass::Allocatable(bool A) const
{
	RealState.Allocatable = A;
}
#endif
void PeripheralTimerClass::Pause() const
{
	if (const uint8_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk < TC_CMR_TCCLKS_XC0)
	{
		_State.TCCLKS = TCCLKS;
		Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | TC_CMR_TCCLKS_XC0;
	}
}
void PeripheralTimerClass::Continue() const
{
	if (Channel.TC_CMR & TC_CMR_TCCLKS_Msk > TC_CMR_TCCLKS_TIMER_CLOCK5)
		Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | _State.TCCLKS;
}
#define HandlerDef(Index)                                                          \
	void TC##Index##_Handler()                                                     \
	{                                                                              \
		PeripheralTimers[(size_t)_PeripheralEnum::Timer##Index]._ClearAndHandle(); \
	}

#ifdef TOFA_TIMER0
HandlerDef(0);
#endif
#ifdef TOFA_TIMER1
HandlerDef(1);
#endif
#ifdef TOFA_TIMER2
HandlerDef(2);
#endif
#ifdef TOFA_TIMER3
HandlerDef(3);
#endif
#ifdef TOFA_TIMER4
HandlerDef(4);
#endif
#ifdef TOFA_TIMER5
HandlerDef(5);
#endif
#ifdef TOFA_TIMER6
HandlerDef(6);
#endif
#ifdef TOFA_TIMER7
HandlerDef(7);
#endif
#ifdef TOFA_TIMER8
HandlerDef(8);
#endif
void PeripheralTimerClass::_ClearAndHandle() const
{
	Channel.TC_SR; // 必须读取一次才能清空中断旗帜
	_State.Handler();
}
void PeripheralTimerClass::Initialize() const
{
	if (_State.Uninitialized)
	{
		PMC->PMC_WPMR = PMC_WPMR_WPKEY_VALUE;
		pmc_enable_periph_clk(UL_ID_TC);
		NVIC_EnableIRQ(irq);
		// TC_WPMR默认不写保护
		_State.Uninitialized = false;
	}
}
void PeripheralTimerClass::StartTiming() const
{
	Initialize();
	Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
	Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
	Channel.TC_IDR = ~TC_IDR_COVFS;
	Channel.TC_IER = TC_IER_COVFS;
	_State.OverflowCount = 0;
	_State.Handler = [this]()
	{
		if (++_State.OverflowCount == 4)
		{
			if (++Channel.TC_CMR & TC_CMR_TCCLKS_Msk == TC_CMR_TCCLKS_TIMER_CLOCK4)
				_State.Handler = [this]()
				{
					if (++_State.OverflowCount == (F_CPU >> 7) / 32768)
					{
						++Channel.TC_CMR;
						_State.OverflowCount = 1;
						_State.Handler = [this]()
						{ _State.OverflowCount++; };
					}
				};
			_State.OverflowCount = 1;
		}
	};
}
Tick PeripheralTimerClass::GetTiming() const
{
	const uint64_t TimeTicks = ((uint64_t)_State.OverflowCount << 32) + Channel.TC_CV;
	uint32_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk;
	if (TCCLKS > TC_CMR_TCCLKS_TIMER_CLOCK5)
		TCCLKS = _State.TCCLKS;
	return TCCLKS == TC_CMR_TCCLKS_TIMER_CLOCK5 ? std::chrono::duration_cast<Tick>(SlowTick(TimeTicks)) : Tick(TimeTicks << (TCCLKS << 1) + 1);
}
void PeripheralTimerClass::Delay(Tick Time) const
{
	Initialize();
	Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
	uint32_t TCCLKS = Time.count() >> 32;
	TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
	Channel.TC_IDR = ~TC_IDR_CPCS;
	Channel.TC_IER = TC_IER_CPCS;
	if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
	{
		// 必须保证不溢出
		TCCLKS = (TCCLKS + 1) >> 1;
		Channel.TC_RC = Time.count() >> (TCCLKS << 1) + 1;
	}
	else
	{
		TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
		const uint64_t SlowTicks = std::chrono::duration_cast<SlowTick>(Time).count();
		Channel.TC_RC = SlowTicks;
		if (volatile uint32_t OverflowCount = SlowTicks >> 32)
		{
			Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
			_State.Handler = [this, &OverflowCount]
			{
				// OverflowCount比实际所需次数少一次，正好利用这一点提前启动CPCDIS
				if (!--OverflowCount)
					Channel.TC_CMR |= TC_CMR_CPCDIS;
			};
			while (Channel.TC_SR & TC_SR_CLKSTA)
				;
			return;
		}
	}
	Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_CPCDIS | TC_CMR_WAVE | TCCLKS;
	while (Channel.TC_SR & TC_SR_CLKSTA)
		;
	return;
}
void PeripheralTimerClass::DoAfter(Tick After, std::function<void()> Do) const
{
	Initialize();
	Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
	uint32_t TCCLKS = After.count() >> 32;
	TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
	Channel.TC_IDR = ~TC_IDR_CPCS;
	Channel.TC_IER = TC_IER_CPCS;
	if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
	{
		// 必须保证不溢出
		TCCLKS = (TCCLKS + 1) >> 1;
		Channel.TC_RC = After.count() >> (TCCLKS << 1) + 1;
	}
	else
	{
		TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
		const uint64_t SlowTicks = std::chrono::duration_cast<SlowTick>(After).count();
		Channel.TC_RC = SlowTicks;
		if (_State.OverflowCount = SlowTicks >> 32)
		{
			Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
			_State.Handler = [this, Do]
			{
				// OverflowCount比实际所需次数少一次，正好利用这一点提前启动CPCDIS
				if (!--_State.OverflowCount)
				{
					Channel.TC_CMR |= TC_CMR_CPCDIS;
					_State.Handler = Do;
				}
			};
			return;
		}
	}
	Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_CPCDIS | TC_CMR_WAVE | TCCLKS;
	_State.Handler = Do;
}
void PeripheralTimerClass::RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const
{
	switch (RepeatTimes)
	{
	case 0:
		DoneCallback();
		break;
	case 1:
		// 重复1次需要特殊处理，因为会导致CPCDIS不可用
		DoAfter(Every, [Do, DoneCallback]()
				{
					Do();
					DoneCallback(); });
		break;
	default:
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		uint32_t TCCLKS = Every.count() >> 32;
		TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
		Channel.TC_IDR = ~TC_IDR_CPCS;
		Channel.TC_IER = TC_IER_CPCS;
		_State.RepeatLeft = RepeatTimes;
		if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
		{
			// 必须保证不溢出
			TCCLKS = (TCCLKS + 1) >> 1;
			Channel.TC_RC = Every.count() >> (TCCLKS << 1) + 1;
		}
		else
		{
			TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
			const uint64_t SlowTicks = std::chrono::duration_cast<SlowTick>(Every).count();
			Channel.TC_RC = SlowTicks;
			if (uint32_t OverflowTarget = SlowTicks >> 32)
			{
				const uint32_t TC_RC = SlowTicks;
				Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
				OverflowTarget++;
				_State.Handler = [this, TC_RC, OverflowTarget, Do, DoneCallback]
				{
					if (!--_State.OverflowCount)
					{
						if (--_State.RepeatLeft)
						{
							_State.OverflowCount = OverflowTarget;
							Channel.TC_RC += TC_RC;
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						Do();
						if (!_State.RepeatLeft)
							DoneCallback();
					}
				};
				return;
			}
		}
		Channel.TC_CMR = TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TCCLKS;
		_State.Handler = [this, Do, DoneCallback]()
		{
			if (--_State.RepeatLeft == 1)
			{
				Channel.TC_CMR |= TC_CMR_CPCDIS;
				_State.Handler = [Do, DoneCallback]()
				{
					Do();
					DoneCallback();
				};
			}
			Do();
		};
	}
}
void PeripheralTimerClass::DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const
{
	switch (NumHalfPeriods)
	{
	case 0:
		DoneCallback();
		break;
	case 1:
		// 重复1次需要特殊处理，因为会导致CPCDIS不可用
		DoAfter(AfterA, [DoA, DoneCallback]()
				{
					DoA();
					DoneCallback(); });
		break;
	default:
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		const Tick PeriodTick = AfterA + AfterB;
		uint32_t TCCLKS = PeriodTick.count() >> 32;
		TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
		_State.RepeatLeft = NumHalfPeriods;
		if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
		{
			// 必须保证不溢出
			TCCLKS = (TCCLKS + 1) >> 1;
			const uint8_t BitShift = (TCCLKS << 1) + 1;
			Channel.TC_RA = AfterA.count() >> BitShift;
			Channel.TC_RC = PeriodTick.count() >> BitShift;
		}
		else
		{
			const uint64_t TimeTicksA = std::chrono::duration_cast<SlowTick>(AfterA).count();
			const uint64_t PeriodSlow = std::chrono::duration_cast<SlowTick>(PeriodTick).count();
			if (PeriodSlow >> 32)
			{
				Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
				const uint64_t TimeTicksB = PeriodSlow - TimeTicksA;
				const uint32_t OverflowTargetA = TimeTicksA >> 32;
				const uint32_t TC_RC_A = TimeTicksA;
				_State.OverflowCount = OverflowTargetA;
				Channel.TC_RC = TC_RC_A;
				const uint32_t OverflowTargetB = TimeTicksB >> 32;
				const uint32_t TC_RC_B = TimeTicksB;
				_State.Handler = [this, TC_RC_B, OverflowTargetB, DoA, DoneCallback]
				{
					if (!--_State.OverflowCount)
					{
						if (--_State.RepeatLeft)
						{
							_State.OverflowCount = OverflowTargetB;
							Channel.TC_RC += TC_RC_B;
							std::swap(_State.Handler, _State.CandidateHandler);
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						DoA();
						if (!_State.RepeatLeft)
							DoneCallback();
					}
				};
				_State.CandidateHandler = [this, TC_RC_A, OverflowTargetA, DoB, DoneCallback]
				{
					if (!--_State.OverflowCount)
					{
						if (--_State.RepeatLeft)
						{
							_State.OverflowCount = OverflowTargetA;
							Channel.TC_RC += TC_RC_A;
							std::swap(_State.Handler, _State.CandidateHandler);
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						DoB();
						if (!_State.RepeatLeft)
							DoneCallback();
					}
				};
				Channel.TC_IDR = ~TC_IDR_CPCS;
				Channel.TC_IER = TC_IER_CPCS;
				return;
			}
			TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
			Channel.TC_RA = TimeTicksA;
			Channel.TC_RC = PeriodSlow;
		}
		Channel.TC_CMR = TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TCCLKS;
		Channel.TC_IDR = ~(TC_IDR_CPAS | TC_IDR_CPCS);
		Channel.TC_IER = TC_IER_CPAS | TC_IER_CPCS;
		_State.Handler = [this, DoA, DoneCallback]()
		{
			if (--_State.RepeatLeft)
				std::swap(_State.Handler, _State.CandidateHandler);
			else
				Channel.TC_CCR = TC_CCR_CLKDIS;
			DoA();
			if (!_State.RepeatLeft)
				DoneCallback();
		};
		_State.CandidateHandler = [this, DoB, DoneCallback]()
		{
			if (--_State.RepeatLeft)
				std::swap(_State.Handler, _State.CandidateHandler);
			else
				Channel.TC_CCR = TC_CCR_CLKDIS;
			DoB();
			if (!_State.RepeatLeft)
				DoneCallback();
		};
	}
}
_PeripheralState Timers_one_for_all::_TimerStates[(uint8_t)_PeripheralEnum::NumPeripherals];
#endif