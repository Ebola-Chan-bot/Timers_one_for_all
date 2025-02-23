Timers_one_for_all::TimerClass const *Timers_one_for_all::AllocateTimer()
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
namespace Timers_one_for_all
{
	// 需要测试：比较寄存器置0能否实现溢出中断？而不会一开始就中断？
	template <uint8_t... Values>
	using _U8Sequence = std::integer_sequence<uint8_t, Values...>;
	template <uint8_t First, typename T>
	struct _U8SequencePrepend;
	template <uint8_t First, uint8_t... Rest>
	struct _U8SequencePrepend<First, _U8Sequence<Rest...>>
	{
		using type = _U8Sequence<First, Rest...>;
	};
	template <typename T>
	struct _U8SequenceDiff;
	template <uint8_t First, uint8_t Second, uint8_t... Rest>
	struct _U8SequenceDiff<_U8Sequence<First, Second, Rest...>>
	{
		using type = typename _U8SequencePrepend<Second - First, typename _U8SequenceDiff<_U8Sequence<Second, Rest...>>::type>::type;
	};
	template <uint8_t First>
	struct _U8SequenceDiff<_U8Sequence<First>>
	{
		using type = _U8Sequence<>;
	};
	// 输入预分频器应排除起始0
	template <typename T, uint8_t BitIndex = 0, uint8_t PrescalerIndex = 1>
	struct _BitLimit
	{
		using type = typename _U8SequencePrepend<PrescalerIndex, typename _BitLimit<T, BitIndex + 1, PrescalerIndex>::type>::type;
	};
	template <uint8_t BitIndex, uint8_t PrescalerIndex, uint8_t... Rest>
	struct _BitLimit<_U8Sequence<BitIndex + 1, Rest...>, BitIndex, PrescalerIndex>
	{
		using type = typename _U8SequencePrepend<PrescalerIndex, typename _BitLimit<_U8Sequence<Rest...>, BitIndex + 1, PrescalerIndex + 1>::type>::type;
	};
	template <uint8_t BitIndex, uint8_t PrescalerIndex>
	struct _BitLimit<_U8Sequence<BitIndex + 1>, BitIndex, PrescalerIndex>
	{
		using type = _U8Sequence<PrescalerIndex>;
	};
	template <typename T>
	struct _PrescalerDiff;
	template <uint8_t... Diff>
	struct _PrescalerDiff<_U8Sequence<Diff...>>
	{
		static constexpr uint8_t AdvanceFactor[] = {1 << Diff...};
	};
	template <typename T>
	struct _BitsToPrescaler_s;
	template <uint8_t... Bit>
	struct _BitsToPrescaler_s<_U8Sequence<Bit...>>
	{
		static constexpr uint8_t BitsToPrescaler[] = {Bit...};
	};
	// 0号是无限大预分频器，表示暂停
	template <uint8_t... BitShift>
	struct _PrescalerType : public _PrescalerDiff<typename _U8SequenceDiff<_U8Sequence<0, BitShift...>>::type>, public _BitsToPrescaler_s<typename _BitLimit<_U8Sequence<BitShift...>>::type>
	{
		static constexpr uint8_t BitShifts[] = {-1, 0, BitShift...};
		static constexpr uint8_t MaxClock = std::extent_v<decltype(BitShifts)> - 1;
		static constexpr uint8_t MaxShift = BitShifts[MaxClock];
	};
	_TimerState _TimerStates[NumTimers];
}
#ifdef TOFA_TIMER0
#warning "使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0"
ISR(TIMER0_COMPA_vect)
{
	(*Timers_one_for_all::_TimerStates[(size_t)Timers_one_for_all::TimerEnum::Timer0].COMPA)();
}
ISR(TIMER0_COMPB_vect)
{
	(*Timers_one_for_all::_TimerStates[(size_t)Timers_one_for_all::TimerEnum::Timer0].COMPB)();
}
#endif
#define _TOFA_TimerIsr(T)                                                                             \
	ISR(TIMER##T##_OVF_vect)                                                                          \
	{                                                                                                 \
		(*Timers_one_for_all::_TimerStates[(size_t)Timers_one_for_all::TimerEnum::Timer##T].OVF)();   \
	}                                                                                                 \
	ISR(TIMER##T##_COMPA_vect)                                                                        \
	{                                                                                                 \
		(*Timers_one_for_all::_TimerStates[(size_t)Timers_one_for_all::TimerEnum::Timer##T].COMPA)(); \
	}                                                                                                 \
	ISR(TIMER##T##_COMPB_vect)                                                                        \
	{                                                                                                 \
		(*Timers_one_for_all::_TimerStates[(size_t)Timers_one_for_all::TimerEnum::Timer##T].COMPB)(); \
	}
#ifdef TOFA_TIMER1
_TOFA_TimerIsr(1);
#endif
#ifdef TOFA_TIMER2
#warning "使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2"
_TOFA_TimerIsr(2);
#endif
#ifdef TOFA_TIMER3
_TOFA_TimerIsr(3);
#endif
#ifdef TOFA_TIMER4
_TOFA_TimerIsr(4);
#endif
#ifdef TOFA_TIMER5
_TOFA_TimerIsr(5);
#endif
namespace Timers_one_for_all
{
	using _Prescaler01 = _PrescalerType<3, 6, 8, 10>;
	using _Prescaler2 = _PrescalerType<3, 5, 6, 7, 8, 10>;
	// 返回OverflowTarget
	template <typename T>
	struct _FirstArgumentType;
	template <typename R, typename FirstArg, typename... Args>
	struct _FirstArgumentType<R(FirstArg, Args...)>
	{
		using type = FirstArg;
	};
	template <typename T>
	using _FirstArgumentType_t = typename _FirstArgumentType<T>::type;
	template <typename Prescaler>
	uint32_t _PrescalerOverflow(uint32_t Cycles, volatile uint8_t &Clock)
	{
		if (const uint32_t OverflowTarget = Cycles >> Prescaler::MaxShift)
		{
			Clock = Prescaler::MaxClock;
			return OverflowTarget;
		}
		else
		{
			Clock = Cycles ? Prescaler::BitsToPrescaler[sizeof(_FirstArgumentType_t<decltype(__builtin_clz)>) * 8 - 1 - __builtin_clz(Cycles)] : 1;
			return 0;
		}
	}
#ifdef TOFA_TIMER0
	bool TimerClass0::Busy() const
	{
		return TIMSK0 & ~(1 << TOIE0);
	}
	void TimerClass0::Stop() const
	{
		TIMSK0 = 1 << TOIE0;
	}
	void TimerClass0::StartTiming() const
	{
		TIMSK0 = 1 << TOIE0;
		TCNT0 = 0;
		TCCR0B = 1;
		TCCR0A = 0;
		_State.OverflowCountA = 0;
		_State.COMPA = &(_State.HandlerA = [&_State = _State]()
						 {
		if (++_State.OverflowCountA == _Prescaler01::AdvanceFactor[TCCR0B]) {
			_State.OverflowCountA = 1;
			if ((++TCCR0B >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
				_State.COMPA = &_State.HandlerB;
		} });
		_State.HandlerB = [&OverflowCount = _State.OverflowCountA]()
		{ OverflowCount++; };
		OCR0A = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
	}
	Tick TimerClass0::GetTiming() const
	{
		const uint8_t Clock = TCCR0B;
		return Tick(((uint64_t)_State.OverflowCountA << 8) + TCNT0 << _Prescaler01::BitShifts[Clock ? Clock : _State.Clock]);
	}
	void TimerClass0::Delay(Tick Time) const
	{
		TIMSK0 = 1 << TOIE0;
		TCNT0 = 0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler01>(Time.count() >> 8, TCCR0B) + 1;
		TCCR0A = 0; // 必须先确保TCCRA低2位全0，否则OCRA会受到限制
		OCR0A = Time.count() >> _Prescaler01::BitShifts[TCCR0B];
		_State.COMPA = &(_State.HandlerA = [&OverflowCount]()
						 {
							 if (!--OverflowCount)
								 TIMSK0 = 1 << TOIE0; // 由中断负责停止计时器，避免OverflowCount下溢
						 });
		TIFR0 = -1;
		TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
		while (OverflowCount)
			;
	}
	void TimerClass0::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		TIMSK0 = 1 << TOIE0;
		TCNT0 = 0;
		_State.OverflowCountA = _PrescalerOverflow<_Prescaler01>(After.count() >> 8, TCCR0B) + 1;
		TCCR0A = 0;
		OCR0A = After.count() >> _Prescaler01::BitShifts[TCCR0B];
		_State.COMPA = &(_State.HandlerA = [&OverflowCount = _State.OverflowCountA, Do = std::move(Do)]()
						 {
		if (!--OverflowCount) {
			TIMSK0 = 1 << TOIE0;
			Do();
		} });
		TIFR0 = -1;
		TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
	}
	void TimerClass0::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (RepeatTimes)
		{
			TIMSK0 = 1 << TOIE0;
			TCNT0 = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(Every.count() >> 8, TCCR0B))
			{
				// 在有溢出情况下，CTC需要多用一个中断且逻辑更复杂，毫无必要，直接用加法更简明
				const uint8_t OcraTarget = Every.count() >> _Prescaler01::MaxShift;
				TCCR0A = 0;
				OCR0A = OcraTarget;
				_State.OverflowCountA = ++OverflowTarget;
				_State.COMPA = &(_State.HandlerA = [&_State = _State, Do = std::move(Do), DoneCallback = std::move(DoneCallback), OverflowTarget, OcraTarget]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft) {
						_State.OverflowCountA = OverflowTarget;
						OCR0A += OcraTarget;
					}
					else
						TIMSK0 = 1 << TOIE0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
			}
			else
			{
				TCCR0A = 1 << WGM01;
				OCR0A = Every.count() >> _Prescaler01::BitShifts[TCCR0B];
				_State.COMPA = &(_State.HandlerA = [&RepeatLeft = _State.RepeatLeft, Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--RepeatLeft)
					TIMSK0 = 1 << TOIE0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!RepeatLeft)
					DoneCallback(); });
			}
			TIFR0 = -1;
			_State.RepeatLeft = RepeatTimes;
			TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
		}
		else
			DoneCallback();
	}
	void TimerClass0::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (NumHalfPeriods)
		{
			TIMSK0 = 1 << TOIE0;
			AfterB += AfterA;
			TCNT0 = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(AfterB.count() >> 8, TCCR0B))
			{
				TCCR0A = 0;
				OCR0A = AfterA.count() >> _Prescaler01::MaxShift;
				_State.OverflowCountA = (AfterA.count() >> _Prescaler01::MaxShift + 8) + 1;
				const uint8_t OcrTarget = AfterB.count() >> _Prescaler01::MaxShift;
				_State.COMPA = &(_State.HandlerA = [&_State = _State, OcrTarget, OverflowTarget, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft > 1) {
						_State.OverflowCountA = OverflowTarget;
						OCR0A += OcrTarget;
					}
					else
						TIMSK0 &= ~(1 << OCIE0A);
					DoA();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
				_State.OverflowCountB = OverflowTarget;
				OCR0B = OcrTarget;
				_State.COMPB = &(_State.HandlerB = [&_State = _State, OcrTarget, OverflowTarget, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
					if (!--_State.OverflowCountA) {
						if (--_State.RepeatLeft > 1) {
							_State.OverflowCountB = OverflowTarget;
							OCR0B += OcrTarget;
						}
						else
							TIMSK0 &= ~(1 << OCIE0B);
						DoB();
						if (!_State.RepeatLeft)
							DoneCallback();
					} });
			}
			else
			{
				const uint8_t BitShift = _Prescaler01::BitShifts[TCCR0B];
				TCCR0A = 1 << WGM01;
				// AB置反，因为DoB需要仅适用于A的CTC
				OCR0B = AfterA.count() >> BitShift;
				_State.COMPB = &(_State.HandlerB = [&RepeatLeft = _State.RepeatLeft, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--RepeatLeft < 2)
					TIMSK0 &= ~(1 << OCIE0B);
				DoA();
				if (!RepeatLeft)
					DoneCallback(); });
				OCR0A = AfterB.count() >> BitShift;
				_State.COMPA = &(_State.HandlerA = [&RepeatLeft = _State.RepeatLeft, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--RepeatLeft < 2)
					TIMSK0 &= ~(1 << OCIE0A);
				DoB();
				if (!RepeatLeft)
					DoneCallback(); });
			}
			_State.RepeatLeft = NumHalfPeriods;
			TIFR0 = -1;
			TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
		}
		else
			DoneCallback();
	}
#endif
#if defined TOFA_TIMER1 || defined TOFA_TIMER3 || defined TOFA_TIMER4 || defined TOFA_TIMER5
	bool TimerClass1::Busy() const
	{
		return TIMSK;
	}
	void TimerClass1::Stop() const
	{
		TIMSK = 0;
	}
	void TimerClass1::StartTiming() const
	{
		TIMSK = 0;
		TCNT = 0;
		TCCRB = 1;
		TCCRA = 0;
		_State.OverflowCountA = 0;
		_State.OVF = &(_State.HandlerA = [this]()
					   {
		if (++_State.OverflowCountA == _Prescaler01::AdvanceFactor[TCCRB]) {
			_State.OverflowCountA = 1;
			if ((++TCCRB >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
				_State.OVF = &_State.HandlerB;
		} });
		_State.HandlerB = [&OverflowCount = _State.OverflowCountA]()
		{ OverflowCount++; };
		TIFR = -1;
		TIMSK = 1 << TOIE1;
	}
	Tick TimerClass1::GetTiming() const
	{
		const uint8_t Clock = TCCRB;
		return Tick(((uint64_t)_State.OverflowCountA << 16) + TCNT << _Prescaler01::BitShifts[Clock ? Clock : _State.Clock]);
	}
	void TimerClass1::Delay(Tick Time) const
	{
		TIMSK = 0;
		TCNT = 0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler01>(Time.count() >> 16, TCCRB) + 1;
		TCCRA = 0;
		OCRA = Time.count() >> _Prescaler01::BitShifts[TCCRB];
		_State.COMPA = &(_State.HandlerA = [&TIMSK = TIMSK, &OverflowCount]()
						 {
		if (!--OverflowCount)
			TIMSK = 0; });
		TIFR = -1;
		TIMSK = 1 << OCIE1A;
		while (OverflowCount)
			;
	}
	void TimerClass1::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		TIMSK = 0;
		TCNT = 0;
		_State.OverflowCountA = _PrescalerOverflow<_Prescaler01>(After.count() >> 16, TCCRB) + 1;
		TCCRA = 0;
		OCRA = After.count() >> _Prescaler01::BitShifts[TCCRB];
		_State.COMPA = &(_State.HandlerA = [this, Do = std::move(Do)]()
						 {
		if (!--_State.OverflowCountA) {
			TIMSK = 0;
			Do();
		} });
		TIFR = -1;
		TIMSK = 1 << OCIE1A;
	}
	void TimerClass1::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (RepeatTimes)
		{
			TIMSK = 0;
			TCNT = 0;
			TCCRA = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(Every.count() >> 16, TCCRB))
			{
				const uint16_t OcraTarget = Every.count() >> _Prescaler01::MaxShift;
				OCRA = OcraTarget;
				_State.OverflowCountA = ++OverflowTarget;
				_State.COMPA = &(_State.HandlerA = [this, Do = std::move(Do), DoneCallback = std::move(DoneCallback), OverflowTarget, OcraTarget]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft) {
						_State.OverflowCountA = OverflowTarget;
						OCRA += OcraTarget;
					}
					else
						TIMSK = 0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
			}
			else
			{
				OCRA = Every.count() >> _Prescaler01::BitShifts[TCCRB];
				_State.COMPA = &(_State.HandlerA = [this, Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--_State.RepeatLeft)
					TIMSK = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!_State.RepeatLeft)
					DoneCallback(); });
				TCCRB |= 1 << WGM12;
			}
			TIFR = -1;
			_State.RepeatLeft = RepeatTimes;
			TIMSK = 1 << OCIE1A;
		}
		else
			DoneCallback();
	}
	void TimerClass1::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (NumHalfPeriods)
		{
			TIMSK = 0;
			AfterB += AfterA;
			TCNT = 0;
			TCCRA = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(AfterB.count() >> 16, TCCRB))
			{
				OCRA = AfterA.count() >> _Prescaler01::MaxShift;
				_State.OverflowCountA = (AfterA.count() >> _Prescaler01::MaxShift + 16) + 1;
				const uint16_t OcrTarget = AfterB.count() >> _Prescaler01::MaxShift;
				_State.COMPA = &(_State.HandlerA = [this, OcrTarget, OverflowTarget, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft > 1) {
						_State.OverflowCountA = OverflowTarget;
						OCRA += OcrTarget;
					}
					else
						TIMSK &= ~(1 << OCIE1A);
					DoA();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
				_State.OverflowCountB = OverflowTarget;
				OCRB = OcrTarget;
				_State.COMPB = &(_State.HandlerB = [this, OcrTarget, OverflowTarget, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
					if (!--_State.OverflowCountA) {
						if (--_State.RepeatLeft > 1) {
							_State.OverflowCountB = OverflowTarget;
							OCRB += OcrTarget;
						}
						else
							TIMSK &= ~(1 << OCIE1B);
						DoB();
						if (!_State.RepeatLeft)
							DoneCallback();
					} });
			}
			else
			{
				const uint8_t BitShift = _Prescaler01::BitShifts[TCCRB];
				// AB置反，因为DoB需要仅适用于A的CTC
				OCRB = AfterA.count() >> BitShift;
				_State.COMPB = &(_State.HandlerB = [this, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--_State.RepeatLeft < 2)
					TIMSK &= ~(1 << OCIE1B);
				DoA();
				if (!_State.RepeatLeft)
					DoneCallback(); });
				OCRA = AfterB.count() >> BitShift;
				_State.COMPA = &(_State.HandlerA = [this, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--_State.RepeatLeft < 2)
					TIMSK &= ~(1 << OCIE1A);
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback(); });
				TCCRB |= 1 << WGM12;
			}
			_State.RepeatLeft = NumHalfPeriods;
			TIFR = -1;
			TIMSK = (1 << OCIE1A) | (1 << OCIE1B);
		}
		else
			DoneCallback();
	}
#endif
#ifdef TOFA_TIMER2
	bool TimerClass2::Busy() const
	{
		return TIMSK2;
	}
	void TimerClass2::Stop() const
	{
		TIMSK2 = 0;
	}
	void TimerClass2::StartTiming() const
	{
		TIMSK2 = 0;
		TCNT2 = 0;
		TCCR2B = 1;
		TCCR2A = 0;
		_State.OverflowCountA = 0;
		_State.OVF = &(_State.HandlerA = [&_State = _State]()
					   {
		if (++_State.OverflowCountA == _Prescaler01::AdvanceFactor[TCCR2B]) {
			_State.OverflowCountA = 1;
			if ((++TCCR2B >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
				_State.OVF = &_State.HandlerB;
		} });
		_State.HandlerB = [&OverflowCount = _State.OverflowCountA]()
		{ OverflowCount++; };
		TIFR2 = -1;
		TIMSK2 = 1 << TOIE1;
	}
	Tick TimerClass2::GetTiming() const
	{
		const uint8_t Clock = TCCR2B;
		return Tick(((uint64_t)_State.OverflowCountA << 8) + TCNT2 << _Prescaler2::BitShifts[Clock ? Clock : _State.Clock]);
	}
	void TimerClass2::Delay(Tick Time) const
	{
		TIMSK2 = 0;
		TCNT2 = 0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler2>(Time.count() >> 8, TCCR2B) + 1;
		TCCR2A = 0;
		OCR2A = Time.count() >> _Prescaler2::BitShifts[TCCR2B];
		_State.COMPA = &(_State.HandlerA = [&OverflowCount]()
						 {
		if (!--OverflowCount)
			TIMSK2 = 0; });
		TIFR2 = -1;
		TIMSK2 = 1 << OCIE2A;
		while (OverflowCount)
			;
	}
	void TimerClass2::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		TIMSK2 = 0;
		TCNT2 = 0;
		_State.OverflowCountA = _PrescalerOverflow<_Prescaler2>(After.count() >> 8, TCCR2B) + 1;
		TCCR2A = 0;
		OCR2A = After.count() >> _Prescaler2::BitShifts[TCCR2B];
		_State.COMPA = &(_State.HandlerA = [&OverflowCount = _State.OverflowCountA, Do = std::move(Do)]()
						 {
		if (!--OverflowCount) {
			TIMSK2 = 0;
			Do();
		} });
		TIFR2 = -1;
		TIMSK2 = 1 << OCIE2A;
	}
	void TimerClass2::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (RepeatTimes)
		{
			TIMSK2 = 0;
			TCNT2 = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler2>(Every.count() >> 8, TCCR2B))
			{
				const uint8_t OcraTarget = Every.count() >> _Prescaler2::MaxShift;
				TCCR2A = 0;
				OCR2A = OcraTarget;
				_State.OverflowCountA = ++OverflowTarget;
				_State.COMPA = &(_State.HandlerA = [&_State = _State, Do = std::move(Do), DoneCallback = std::move(DoneCallback), OverflowTarget, OcraTarget]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft) {
						_State.OverflowCountA = OverflowTarget;
						OCR2A += OcraTarget;
					}
					else
						TIMSK2 = 0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
			}
			else
			{
				TCCR2A = 1 << WGM21;
				OCR2A = Every.count() >> _Prescaler2::BitShifts[TCCR2B];
				_State.COMPA = &(_State.HandlerA = [&RepeatLeft = _State.RepeatLeft, Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--RepeatLeft)
					TIMSK2 = 0;
				// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
				Do();
				if (!RepeatLeft)
					DoneCallback(); });
			}
			TIFR2 = -1;
			_State.RepeatLeft = RepeatTimes;
			TIMSK2 = 1 << OCIE2A;
		}
		else
			DoneCallback();
	}
	void TimerClass2::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (NumHalfPeriods)
		{
			TIMSK2 = 0;
			AfterB += AfterA;
			TCNT2 = 0;
			if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler2>(AfterB.count() >> 8, TCCR2B))
			{
				TCCR2A = 0;
				OCR2A = AfterA.count() >> _Prescaler2::MaxShift;
				_State.OverflowCountA = (AfterA.count() >> _Prescaler2::MaxShift + 8) + 1;
				const uint8_t OcrTarget = AfterB.count() >> _Prescaler2::MaxShift;
				_State.COMPA = &(_State.HandlerA = [&_State = _State, OcrTarget, OverflowTarget, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (!--_State.OverflowCountA) {
					if (--_State.RepeatLeft > 1) {
						_State.OverflowCountA = OverflowTarget;
						OCR2A += OcrTarget;
					}
					else
						TIMSK2 &= ~(1 << OCIE2A);
					DoA();
					if (!_State.RepeatLeft)
						DoneCallback();
				} });
				_State.OverflowCountB = OverflowTarget;
				OCR2B = OcrTarget;
				_State.COMPB = &(_State.HandlerB = [&_State = _State, OcrTarget, OverflowTarget, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
					if (!--_State.OverflowCountA) {
						if (--_State.RepeatLeft > 1) {
							_State.OverflowCountB = OverflowTarget;
							OCR2B += OcrTarget;
						}
						else
							TIMSK2 &= ~(1 << OCIE2B);
						DoB();
						if (!_State.RepeatLeft)
							DoneCallback();
					} });
			}
			else
			{
				const uint8_t BitShift = _Prescaler2::BitShifts[TCCR2B];
				TCCR2A = 1 << WGM21;
				// AB置反，因为DoB需要仅适用于A的CTC
				OCR2B = AfterA.count() >> BitShift;
				_State.COMPB = &(_State.HandlerB = [&RepeatLeft = _State.RepeatLeft, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--RepeatLeft < 2)
					TIMSK2 &= ~(1 << OCIE2B);
				DoA();
				if (!RepeatLeft)
					DoneCallback(); });
				OCR2A = AfterB.count() >> BitShift;
				_State.COMPA = &(_State.HandlerA = [&RepeatLeft = _State.RepeatLeft, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
								 {
				if (--RepeatLeft < 2)
					TIMSK2 &= ~(1 << OCIE2A);
				DoB();
				if (!RepeatLeft)
					DoneCallback(); });
			}
			_State.RepeatLeft = NumHalfPeriods;
			TIFR2 = -1;
			TIMSK2 = (1 << OCIE2A) | (1 << OCIE2B);
		}
		else
			DoneCallback();
	}
#endif
}
#endif
#ifdef ARDUINO_ARCH_SAM
namespace Timers_one_for_all
{
#ifdef TOFA_SYSTIMER
	// 系统计时器实测不符合文档描述。设置 CTRL ENABLE 不能将LOAD载入VAL，只能手动设置VAL，但只能将其设为0，设为0以外的任何值都会自动变成0。用这种方式使得VAL变为0时，不会触发中断。VAL载入LOAD需要不确定的时间，需要循环等待VAL已载入后才能修改LOAD，否则旧LOAD可能会被丢弃。这个载入时间与CLKSOURCE无关。VAL载入LOAD发生在中断之前，所以在中断中只能设置下次LOAD值
	static struct _SystemTimerState : public _TimerState
	{
		uint32_t VAL;
		std::move_only_function<void() const> HandlerA;
		std::move_only_function<void() const> HandlerB;
		uint32_t OverflowCount;
		_SystemTimerState() { Handler = nullptr; }
	} _SystemState;
	extern "C" int sysTickHook()
	{
		if (_SystemState.Handler)
			(*_SystemState.Handler)();
		return 0;
	}
	bool SystemTimerClass::Busy() const
	{
		return SysTick->CTRL & SysTick_CTRL_TICKINT_Msk;
	}
	void SystemTimerClass::Pause() const
	{
		if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
			SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}
	void SystemTimerClass::Continue() const
	{
		if ((SysTick->CTRL & (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk)) == SysTick_CTRL_TICKINT_Msk)
			SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}
	void SystemTimerClass::Stop() const
	{
		SysTick->CTRL = 0; // 不能简单将Handler设为nullptr，可能陷入无限中断
	}
	constexpr uint32_t _SysTick_CTRL_MCK8 = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	constexpr uint32_t _SysTick_CTRL_MCK = SysTick_CTRL_CLKSOURCE_Msk | _SysTick_CTRL_MCK8;
	void SystemTimerClass::StartTiming() const
	{
		SysTick->VAL = 0;
		SysTick->LOAD = -1;
		SysTick->CTRL = _SysTick_CTRL_MCK;
		_SystemState.OverflowCount = 8;
		_SystemState.HandlerB = []()
		{ _SystemState.OverflowCount++; };
		_SystemState.Handler = &(_SystemState.HandlerA = []()
								 {
		if (!--_SystemState.OverflowCount) {
			SysTick->CTRL = _SysTick_CTRL_MCK8;
			_SystemState.OverflowCount = 1;
			_SystemState.Handler = &_SystemState.HandlerB;
		} });
	}
	Tick SystemTimerClass::GetTiming() const
	{
		return Tick(((uint64_t)(_SystemState.OverflowCount + 1) << 24) - SysTick->VAL << (SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk ? 0 : 3));
	}
	void SystemTimerClass::Delay(Tick Time) const
	{
		SysTick->CTRL = 0;
		SysTick->VAL = 0;
		uint64_t TimeTicks = Time.count();
		uint32_t CTRL = _SysTick_CTRL_MCK;
		if (TimeTicks >> 24)
		{
			TimeTicks >>= 3;
			if (volatile uint32_t OverflowCount = TimeTicks >> 24)
			{
				OverflowCount++;
				_SystemState.Handler = &(_SystemState.HandlerA = [&OverflowCount]()
										 { if (!--OverflowCount)
				SysTick->CTRL = 0; });
				SysTick->LOAD = TimeTicks;
				SysTick->CTRL = _SysTick_CTRL_MCK8;
				while (!SysTick->VAL)
					;
				SysTick->LOAD = -1;
				while (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
					;
				return;
			}
			CTRL = _SysTick_CTRL_MCK8;
		}
		_SystemState.Handler = &(_SystemState.HandlerA = []()
								 { SysTick->CTRL = 0; });
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
		while (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
			;
	}
	void SystemTimerClass::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		SysTick->CTRL = 0;
		SysTick->VAL = 0;
		uint64_t TimeTicks = After.count();
		uint32_t CTRL = _SysTick_CTRL_MCK;
		Do = [Do = std::move(Do)]()
		{
			SysTick->CTRL = 0;
			Do();
		};
		if (TimeTicks >> 24)
		{
			TimeTicks >>= 3;
			if (_SystemState.OverflowCount = TimeTicks >> 24)
			{
				_SystemState.Handler = &(_SystemState.HandlerA = [Do = std::move(Do)]()
										 {
				if (!--_SystemState.OverflowCount)
					_SystemState.Handler = &Do; });
				SysTick->LOAD = TimeTicks;
				SysTick->CTRL = _SysTick_CTRL_MCK8;
				while (!SysTick->VAL)
					;
				SysTick->LOAD = -1;
				return;
			}
			CTRL = _SysTick_CTRL_MCK8;
		}
		_SystemState.Handler = &(_SystemState.HandlerA = std::move(Do));
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
	}
	void SystemTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (RepeatTimes)
		{
			SysTick->CTRL = 0;
			SysTick->VAL = 0;
			uint64_t TimeTicks = Every.count();
			uint32_t CTRL = _SysTick_CTRL_MCK;
			_SystemState.RepeatLeft = RepeatTimes;
			if (TimeTicks >> 24)
			{
				TimeTicks >>= 3;
				if (uint32_t OverflowTarget = TimeTicks >> 24)
				{
					const uint32_t LOAD = TimeTicks;
					OverflowTarget++;
					_SystemState.OverflowCount = OverflowTarget;
					_SystemState.Handler = &(_SystemState.HandlerA = [Do = std::move(Do), OverflowTarget, LOAD, DoneCallback = std::move(DoneCallback)]()
											 {
					switch (--_SystemState.OverflowCount) {
					case 0:
						if (--_SystemState.RepeatLeft) {
							SysTick->LOAD = -1;
							_SystemState.OverflowCount = OverflowTarget;
						}
						else
							SysTick->CTRL = 0;
						Do();
						if (!_SystemState.RepeatLeft)
							DoneCallback();
						break;
					case 1:
						SysTick->LOAD = LOAD;
					} });
					SysTick->LOAD = LOAD;
					SysTick->CTRL = _SysTick_CTRL_MCK8;
					while (!SysTick->VAL)
						;
					SysTick->LOAD = -1;
					return;
				}
				CTRL = _SysTick_CTRL_MCK8;
			}
			_SystemState.Handler = &(_SystemState.HandlerA = [Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
									 {
			if (!--_SystemState.RepeatLeft)
				SysTick->CTRL = 0;
			Do();
			if (!_SystemState.RepeatLeft)
				DoneCallback(); });
			SysTick->LOAD = TimeTicks;
			SysTick->CTRL = CTRL;
		}
		else
			DoneCallback();
	}
	void SystemTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (NumHalfPeriods)
		{
			SysTick->CTRL = 0;
			SysTick->VAL = 0;
			uint64_t TimeTicksA = AfterA.count();
			uint64_t TimeTicksB = AfterB.count();
			uint32_t CTRL = _SysTick_CTRL_MCK;
			SysTick->LOAD = -1;
			if (max(TimeTicksA, TimeTicksB) >> 24)
			{
				TimeTicksA >>= 3;
				TimeTicksB >>= 3;
				uint32_t OverflowTargetA = (TimeTicksA >> 24);
				uint32_t OverflowTargetB = (TimeTicksB >> 24);
				if (max(OverflowTargetA, OverflowTargetB))
				{
					const uint32_t TicksLeftA = TimeTicksA;
					const uint32_t TicksLeftB = TimeTicksB;
					if (OverflowTargetA)
					{
						_SystemState.OverflowCount = ++OverflowTargetA;
						if (OverflowTargetB)
						{
							OverflowTargetB++;
							_SystemState.HandlerA = [TicksLeftB, OverflowTargetB, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
							{
								switch (--_SystemState.OverflowCount)
								{
								case 0:
									if (--_SystemState.RepeatLeft)
									{
										SysTick->LOAD = -1;
										_SystemState.OverflowCount = OverflowTargetB;
										_SystemState.Handler = &_SystemState.HandlerB;
									}
									else
										SysTick->CTRL = 0;
									DoA();
									if (!_SystemState.RepeatLeft)
										DoneCallback();
									break;
								case 1:
									SysTick->LOAD = TicksLeftB;
								}
							};
							_SystemState.HandlerB = [TicksLeftA, OverflowTargetA, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
							{
								switch (--_SystemState.OverflowCount)
								{
								case 0:
									if (--_SystemState.RepeatLeft)
									{
										SysTick->LOAD = -1;
										_SystemState.OverflowCount = OverflowTargetA;
										_SystemState.Handler = &_SystemState.HandlerA;
									}
									else
										SysTick->CTRL = 0;
									DoB();
									if (!_SystemState.RepeatLeft)
										DoneCallback();
									break;
								case 1:
									SysTick->LOAD = TicksLeftA;
								}
							};
						}
						else
						{
							_SystemState.HandlerA = [TicksLeftB, TicksLeftA, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
							{
								switch (--_SystemState.OverflowCount)
								{
								case 0:
									if (--_SystemState.RepeatLeft)
									{
										SysTick->LOAD = TicksLeftA;
										_SystemState.Handler = &_SystemState.HandlerB;
									}
									else
										SysTick->CTRL = 0;
									DoA();
									if (!_SystemState.RepeatLeft)
										DoneCallback();
									break;
								case 1:
									SysTick->LOAD = TicksLeftB;
								}
							};
							_SystemState.HandlerB = [OverflowTargetA, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
							{
								if (--_SystemState.RepeatLeft)
								{
									SysTick->LOAD = -1;
									_SystemState.OverflowCount = OverflowTargetA;
									_SystemState.Handler = &_SystemState.HandlerA;
								}
								else
									SysTick->CTRL = 0;
								DoB();
								if (!_SystemState.RepeatLeft)
									DoneCallback();
							};
						}
					}
					else
					{
						_SystemState.HandlerA = [OverflowTargetB, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
						{
							if (--_SystemState.RepeatLeft)
							{
								SysTick->LOAD = -1;
								_SystemState.OverflowCount = OverflowTargetB;
								_SystemState.Handler = &_SystemState.HandlerB;
							}
							else
								SysTick->CTRL = 0;
							DoA();
							if (!_SystemState.RepeatLeft)
								DoneCallback();
						};
						_SystemState.HandlerB = [TicksLeftA, TicksLeftB, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
						{
							switch (--_SystemState.OverflowCount)
							{
							case 0:
								if (--_SystemState.RepeatLeft)
								{
									SysTick->LOAD = TicksLeftB;
									_SystemState.Handler = &_SystemState.HandlerA;
								}
								else
									SysTick->CTRL = 0;
								DoB();
								if (!_SystemState.RepeatLeft)
									DoneCallback();
								break;
							case 1:
								SysTick->LOAD = TicksLeftA;
							}
						};
					}
					_SystemState.Handler = &_SystemState.HandlerA;
					SysTick->LOAD = TicksLeftA;
					SysTick->CTRL = _SysTick_CTRL_MCK8;
					while (!SysTick->VAL)
						;
					SysTick->LOAD = OverflowTargetA ? -1 : TicksLeftB;
					return;
				}
				CTRL = _SysTick_CTRL_MCK8;
			}
			const uint32_t TicksLeftA = TimeTicksA;
			const uint32_t TicksLeftB = TimeTicksB;
			_SystemState.Handler = &(_SystemState.HandlerA = [TicksLeftA, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
									 {
			if (--_SystemState.RepeatLeft) {
				SysTick->LOAD = TicksLeftA;
				_SystemState.Handler = &_SystemState.HandlerB;
			}
			else
				SysTick->CTRL = 0;
			DoA();
			if (!_SystemState.RepeatLeft)
				DoneCallback(); });
			_SystemState.HandlerB = [TicksLeftB, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
			{
				if (--_SystemState.RepeatLeft)
				{
					SysTick->LOAD = TicksLeftB;
					_SystemState.Handler = &_SystemState.HandlerA;
				}
				else
					SysTick->CTRL = 0;
				DoB();
				if (!_SystemState.RepeatLeft)
					DoneCallback();
			};
			SysTick->LOAD = TicksLeftA;
			SysTick->CTRL = CTRL;
			while (!SysTick->VAL)
				;
			SysTick->LOAD = TicksLeftB;
		}
		else
			DoneCallback();
	}
	bool SystemTimerClass::Allocatable() const
	{
		return _SystemState.Allocatable;
	}
	void SystemTimerClass::Allocatable(bool A) const
	{
		_SystemState.Allocatable = A;
	}
#endif
using _SlowTick = std::chrono::duration<uint64_t, std::ratio<1, 29400>>; // 数据表说32768，实测明显不准。采用实测值。
#ifdef TOFA_REALTIMER
	static struct _RealTimerState : public _TimerState
	{
		uint16_t OverflowCount;
		std::move_only_function<void() const> HandlerA;
		std::move_only_function<void() const> HandlerB;
		_RealTimerState() { Handler = nullptr; }
	} _RealState;
	extern "C" void RTT_Handler()
	{
		if (_RealState.Handler)
			(*_RealState.Handler)();
		while (RTT->RTT_SR) // 读SR后不会马上清零，必须确认清零后才能退出，否则可能导致无限中断
			;
	}
	bool RealTimerClass::Busy() const
	{
		return (bool)_RealState.Handler;
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
		if (!(RTT->RTT_MR & RTT_MR_ALMIEN) && _RealState.Handler)
			RTT->RTT_MR |= RTT_MR_ALMIEN | RTT_MR_RTTRST;
		// 可能存在问题：RTTRST可能导致AR变成-1
	}
	void RealTimerClass::Stop() const
	{
		_RealState.Handler = nullptr;
		RTT->RTT_MR = 0;
	}
	void RealTimerClass::StartTiming() const
	{
		RTT->RTT_AR = -1;
		RTT->RTT_MR = 1 | RTT_MR_ALMIEN | RTT_MR_RTTRST;
		_RealState.OverflowCount = 0;
		_RealState.Handler = &(_RealState.HandlerA = []()
							  {
								  if (++_RealState.OverflowCount == 2)
								  {
									  const uint16_t RTPRES = RTT->RTT_MR << 1;
									  RTT->RTT_MR = RTPRES | RTT_MR_ALMIEN;
									  // 可能存在问题：MR的修改可能在RTTRST之前不会生效
									  _RealState.OverflowCount = 1;
									  if (!RTPRES)
										  _RealState.Handler = &_RealState.HandlerB;
								  }
								  // 可能存在问题：中断寄存器尚未清空导致中断再次触发
							  });
		_RealState.HandlerB = []()
		{ _RealState.OverflowCount++; };
		while (RTT->RTT_SR)
			;
		NVIC_EnableIRQ(RTT_IRQn);
	}
	Tick RealTimerClass::GetTiming() const
	{
		uint64_t TimerTicks = ((uint64_t)(_RealState.OverflowCount + 1) << 32) + RTT->RTT_VR - RTT->RTT_AR;
		if (const uint16_t RTPRES = RTT->RTT_MR)
			TimerTicks *= RTPRES;
		else
			TimerTicks <<= 16;
		return std::chrono::duration_cast<Tick>(_SlowTick(TimerTicks));
	}
	void RealTimerClass::Delay(Tick Time) const
	{
		RTT->RTT_MR = 0;
		const uint64_t TimerTicks = std::chrono::duration_cast<_SlowTick>(Time).count();
		volatile uint16_t OverflowCount = 1;
		if (const uint32_t RTPRES = TimerTicks >> 32)
		{
			if ((OverflowCount += RTPRES >> 16) > 1)
			{
				RTT->RTT_AR = TimerTicks >> 16;
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
			}
			else
			{
				RTT->RTT_AR = -1;
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
			}
		}
		else
		{
			RTT->RTT_AR = TimerTicks;
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
		}
		_RealState.Handler = &(_RealState.HandlerA = [&OverflowCount]()
							  {
		if (!--OverflowCount) {
			_RealState.Handler = nullptr;
			RTT->RTT_MR = 0;
		} });
		NVIC_EnableIRQ(RTT_IRQn);
		while (OverflowCount)
			;
	}
	void RealTimerClass::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		RTT->RTT_MR = 0;
		Do = [Do = std::move(Do)]()
		{
			_RealState.Handler = nullptr;
			RTT->RTT_MR = 0;
			Do();
		};
		const uint64_t TimerTicks = std::chrono::duration_cast<_SlowTick>(After).count();
		while (RTT->RTT_SR)
			;
		NVIC_EnableIRQ(RTT_IRQn);
		if (const uint32_t RTPRES = TimerTicks >> 32)
		{
			if (_RealState.OverflowCount = RTPRES >> 16)
			{
				RTT->RTT_AR = TimerTicks >> 16;
				_RealState.Handler = &(_RealState.HandlerA = [Do = std::move(Do)]()
									  {
				if (!--_RealState.OverflowCount)
					Do(); });
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				return;
			}
			else
			{
				RTT->RTT_AR = -1;
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
			}
		}
		else
		{
			RTT->RTT_AR = TimerTicks;
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
		}
		_RealState.Handler = &(_RealState.HandlerA = std::move(Do));
	}
	void RealTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (RepeatTimes)
		{
			RTT->RTT_MR = 0;
			const uint64_t TimerTicks = std::chrono::duration_cast<_SlowTick>(Every).count();
			if (const uint32_t RTPRES = TimerTicks >> 32)
			{
				if (uint16_t OverflowTarget = RTPRES >> 16)
				{
					const uint32_t RTT_AR = TimerTicks >> 16;
					RTT->RTT_AR = RTT_AR;
					_RealState.OverflowCount = ++OverflowTarget;
					_RealState.Handler = &(_RealState.HandlerA = [Do = std::move(Do), RTT_AR, OverflowTarget, DoneCallback = std::move(DoneCallback)]()
										  {
					if (!--_RealState.OverflowCount) {
						if (--_RealState.RepeatLeft) {
							RTT->RTT_AR = RTT_AR; // 伪暂停算法可能修改AR，所以每次都要重新设置
							RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
							_RealState.OverflowCount = OverflowTarget;
						}
						else {
							RTT->RTT_MR = 0;
							_RealState.Handler = nullptr;
						}
						Do();
						if (!_RealState.RepeatLeft)
							DoneCallback();
					} });
					RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				}
				else
				{
					RTT->RTT_AR = -1;
					RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
					_RealState.Handler = &(_RealState.HandlerA = [Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
										  {
					if (!--_RealState.RepeatLeft) {
						RTT->RTT_MR = 0;
						_RealState.Handler = nullptr;
					}
					Do();
					if (!_RealState.RepeatLeft)
						DoneCallback(); });
				}
			}
			else
			{
				const uint32_t RTT_AR = TimerTicks;
				RTT->RTT_AR = RTT_AR;
				_RealState.Handler = &(_RealState.HandlerA = [Do = std::move(Do), DoneCallback = std::move(DoneCallback), RTT_AR]()
									  {
				if (--_RealState.RepeatLeft)
					RTT->RTT_AR += RTT_AR;
				else {
					RTT->RTT_MR = 0;
					_RealState.Handler = nullptr;
				}
				Do();
				if (!_RealState.RepeatLeft)
					DoneCallback(); });
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
			}
			_RealState.RepeatLeft = RepeatTimes;
			while (RTT->RTT_SR)
				;
			NVIC_EnableIRQ(RTT_IRQn);
		}
		else
			DoneCallback();
	}
	void RealTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		if (NumHalfPeriods)
		{
			RTT->RTT_MR = 0;
			const uint64_t TimerTicksA = std::chrono::duration_cast<_SlowTick>(AfterA).count();
			const uint64_t TimerTicksB = std::chrono::duration_cast<_SlowTick>(AfterB).count();
			const uint64_t MaxTicks = max(TimerTicksA, TimerTicksB);
			_RealState.RepeatLeft = NumHalfPeriods;
			uint32_t RTPRES = MaxTicks >> 32;
			uint32_t RTT_AR_A;
			uint32_t RTT_AR_B;
			uint32_t RTT_MR;
			while (RTT->RTT_SR)
				;
			NVIC_EnableIRQ(RTT_IRQn);
			if (RTPRES)
			{
				if (RTPRES >> 16)
				{
					RTT_AR_A = TimerTicksA >> 16;
					RTT->RTT_AR = RTT_AR_A;
					RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
					const uint16_t OverflowTargetA = (TimerTicksA >> 48) + 1;
					_RealState.OverflowCount = OverflowTargetA;
					const uint16_t OverflowTargetB = (TimerTicksB >> 48) + 1;
					RTT_AR_B = TimerTicksB >> 16;
					_RealState.Handler = &(_RealState.HandlerA = [OverflowTargetB, RTT_AR_B, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
										  {
					if (!--_RealState.OverflowCount) {
						if (--_RealState.RepeatLeft) {
							_RealState.OverflowCount += OverflowTargetB;
							RTT->RTT_AR += RTT_AR_B;
							_RealState.Handler = &_RealState.HandlerB;
						}
						else {
							RTT->RTT_MR = 0;
							_RealState.Handler = nullptr;
						}
						DoA();
						if (!_RealState.RepeatLeft)
							DoneCallback();
					} });
					_RealState.HandlerB = [OverflowTargetA, RTT_AR_A, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
					{
						if (!--_RealState.OverflowCount)
						{
							if (--_RealState.RepeatLeft)
							{
								_RealState.OverflowCount += OverflowTargetA;
								RTT->RTT_AR += RTT_AR_A;
								_RealState.Handler = &_RealState.HandlerA;
							}
							else
							{
								RTT->RTT_MR = 0;
								_RealState.Handler = nullptr;
							}
							DoB();
							if (!_RealState.RepeatLeft)
								DoneCallback();
						}
					};
					RTT->RTT_MR = RTT_MR;
					return;
				}
				RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
				RTT_AR_A = TimerTicksA / RTPRES;
				RTT_AR_B = TimerTicksB / RTPRES;
			}
			else
			{
				RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
				RTT_AR_A = TimerTicksA;
				RTT_AR_B = TimerTicksB;
			}
			RTT->RTT_AR = RTT_AR_A;
			_RealState.Handler = &(_RealState.HandlerA = [RTT_AR_B, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
								  {
			if (--_RealState.RepeatLeft) {
				RTT->RTT_AR += RTT_AR_B;
				_RealState.Handler = &_RealState.HandlerB;
			}
			else {
				RTT->RTT_MR = 0;
				_RealState.Handler = nullptr;
			}
			DoA();
			if (!_RealState.RepeatLeft)
				DoneCallback(); });
			_RealState.HandlerB = [RTT_AR_A, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
			{
				if (--_RealState.RepeatLeft)
				{
					RTT->RTT_AR += RTT_AR_A;
					_RealState.Handler = &_RealState.HandlerA;
				}
				else
				{
					RTT->RTT_MR = 0;
					_RealState.Handler = nullptr;
				}
				DoB();
				if (!_RealState.RepeatLeft)
					DoneCallback();
			};
			RTT->RTT_MR = RTT_MR;
		}
		else
			DoneCallback();
	}
	bool RealTimerClass::Allocatable() const
	{
		return _RealState.Allocatable;
	}
	void RealTimerClass::Allocatable(bool A) const
	{
		_RealState.Allocatable = A;
	}
#endif
	void PeripheralTimerClass::Pause() const
	{
		const uint8_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk;
		if (TCCLKS < TC_CMR_TCCLKS_XC0)
		{
			_State.TCCLKS = TCCLKS;
			Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | TC_CMR_TCCLKS_XC0;
		}
	}
	void PeripheralTimerClass::Continue() const
	{
		if ((Channel.TC_CMR & TC_CMR_TCCLKS_Msk) > TC_CMR_TCCLKS_TIMER_CLOCK5)
			Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | _State.TCCLKS;
	}
#define _TOFA_HandlerDef(Index)                                                    \
	extern "C" void TC##Index##_Handler()                                          \
	{                                                                              \
		PeripheralTimers[(size_t)_PeripheralEnum::Timer##Index]._ClearAndHandle(); \
	}

#ifdef TOFA_TIMER0
	_TOFA_HandlerDef(0);
#endif
#ifdef TOFA_TIMER1
	_TOFA_HandlerDef(1);
#endif
#ifdef TOFA_TIMER2
	_TOFA_HandlerDef(2);
#endif
#ifdef TOFA_TIMER3
	_TOFA_HandlerDef(3);
#endif
#ifdef TOFA_TIMER4
	_TOFA_HandlerDef(4);
#endif
#ifdef TOFA_TIMER5
	_TOFA_HandlerDef(5);
#endif
#ifdef TOFA_TIMER6
	_TOFA_HandlerDef(6);
#endif
#ifdef TOFA_TIMER7
	_TOFA_HandlerDef(7);
#endif
#ifdef TOFA_TIMER8
	_TOFA_HandlerDef(8);
#endif
	void PeripheralTimerClass::_ClearAndHandle() const
	{
		Channel.TC_SR; // 必须读取一次才能清空中断旗帜
		(*_State.Handler)();
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
		Channel.TC_IDR = -1;
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
		_State.OverflowCount = 0;
		_State.Handler = &(_State.HandlerA = [this]()
						   {
		if (++_State.OverflowCount == 4) {
			if ((++Channel.TC_CMR & TC_CMR_TCCLKS_Msk) == TC_CMR_TCCLKS_TIMER_CLOCK4)
				_State.Handler = &_State.HandlerB;
			_State.OverflowCount = 1;
		} });
		_State.HandlerB = [this]()
		{
			if (++_State.OverflowCount == (F_CPU >> 7) / 32768)
			{
				++Channel.TC_CMR;
				_State.OverflowCount = 1;
				_State.Handler = &(_State.HandlerA = [&OverflowCount = _State.OverflowCount]()
								   { OverflowCount++; });
			}
		};
		Channel.TC_IER = TC_IER_COVFS;
	}
	Tick PeripheralTimerClass::GetTiming() const
	{
		const uint64_t TimeTicks = ((uint64_t)_State.OverflowCount << 32) + Channel.TC_CV;
		uint32_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk;
		if (TCCLKS > TC_CMR_TCCLKS_TIMER_CLOCK5)
			TCCLKS = _State.TCCLKS;
		return TCCLKS == TC_CMR_TCCLKS_TIMER_CLOCK5 ? std::chrono::duration_cast<Tick>(_SlowTick(TimeTicks)) : Tick(TimeTicks << (TCCLKS << 1) + 1);
	}
	void PeripheralTimerClass::Delay(Tick Time) const
	{
		Channel.TC_IDR = -1;
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		uint32_t TCCLKS = Time.count() >> 32;
		TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
		if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
		{
			// 必须保证不溢出
			TCCLKS = (TCCLKS + 1) >> 1;
			Channel.TC_RC = Time.count() >> (TCCLKS << 1) + 1;
		}
		else
		{
			TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
			const uint64_t SlowTicks = std::chrono::duration_cast<_SlowTick>(Time).count();
			Channel.TC_RC = SlowTicks;
			if (volatile uint32_t OverflowCount = SlowTicks >> 32)
			{
				Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
				_State.Handler = &(_State.HandlerA = [this, &OverflowCount]
								   {
				// OverflowCount比实际所需次数少一次，正好利用这一点提前启动CPCDIS
				if (!--OverflowCount)
					Channel.TC_CMR |= TC_CMR_CPCDIS; });
				Channel.TC_IER = TC_IER_CPCS;
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
	void PeripheralTimerClass::DoAfter(Tick After, std::move_only_function<void() const> &&Do) const
	{
		Channel.TC_IDR = -1;
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		uint32_t TCCLKS = After.count() >> 32;
		TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
		if (TCCLKS < (TC_CMR_TCCLKS_TIMER_CLOCK4 << 1) + 1)
		{
			// 必须保证不溢出
			TCCLKS = (TCCLKS + 1) >> 1;
			Channel.TC_RC = After.count() >> (TCCLKS << 1) + 1;
		}
		else
		{
			TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
			const uint64_t SlowTicks = std::chrono::duration_cast<_SlowTick>(After).count();
			Channel.TC_RC = SlowTicks;
			if (_State.OverflowCount = SlowTicks >> 32)
			{
				Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
				_State.Handler = &(_State.HandlerA = [this, Do = std::move(Do)]
								   {
				// OverflowCount比实际所需次数少一次，正好利用这一点提前启动CPCDIS
				if (!--_State.OverflowCount) {
					Channel.TC_CMR |= TC_CMR_CPCDIS;
					_State.Handler = &Do;
				} });
			}
		}
		Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_CPCDIS | TC_CMR_WAVE | TCCLKS;
		_State.Handler = &(_State.HandlerA = std::move(Do));
		Channel.TC_IER = TC_IER_CPCS;
	}
	void PeripheralTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) const
	{
		switch (RepeatTimes)
		{
		case 0:
			DoneCallback();
			break;
		case 1:
			// 重复1次需要特殊处理，因为会导致CPCDIS不可用
			DoAfter(Every, [Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
					{
			Do();
			DoneCallback(); });
			break;
		default:
			Channel.TC_IDR = -1;
			Initialize();
			Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
			uint32_t TCCLKS = Every.count() >> 32;
			TCCLKS = TCCLKS ? sizeof(TCCLKS) * 8 - 1 - __builtin_clz(TCCLKS) : 0; // TCCLKS为0时需要避免下溢
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
				const uint64_t SlowTicks = std::chrono::duration_cast<_SlowTick>(Every).count();
				Channel.TC_RC = SlowTicks;
				if (uint32_t OverflowTarget = SlowTicks >> 32)
				{
					const uint32_t TC_RC = SlowTicks;
					Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
					OverflowTarget++;
					_State.Handler = &(_State.HandlerA = [this, TC_RC, OverflowTarget, Do = std::move(Do), DoneCallback = std::move(DoneCallback)]
									   {
					if (!--_State.OverflowCount) {
						if (--_State.RepeatLeft) {
							_State.OverflowCount = OverflowTarget;
							Channel.TC_RC += TC_RC;
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						Do();
						if (!_State.RepeatLeft)
							DoneCallback();
					} });
					Channel.TC_IER = TC_IER_CPCS;
					return;
				}
			}
			Channel.TC_CMR = TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TCCLKS;
			_State.Handler = &(_State.HandlerA = [this, Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
							   {
			if (--_State.RepeatLeft == 1) {
				Channel.TC_CMR |= TC_CMR_CPCDIS;
				_State.Handler = &_State.HandlerB;
			}
			Do(); });
			_State.HandlerB = [Do = std::move(Do), DoneCallback = std::move(DoneCallback)]()
			{
				Do();
				DoneCallback();
			};
			Channel.TC_IER = TC_IER_CPCS;
		}
	}
	void PeripheralTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) const
	{
		switch (NumHalfPeriods)
		{
		case 0:
			DoneCallback();
			break;
		case 1:
			// 重复1次需要特殊处理，因为会导致CPCDIS不可用
			DoAfter(AfterA, [DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
					{
			DoA();
			DoneCallback(); });
			break;
		default:
			Channel.TC_IDR = -1;
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
				const uint64_t TimeTicksA = std::chrono::duration_cast<_SlowTick>(AfterA).count();
				const uint64_t PeriodSlow = std::chrono::duration_cast<_SlowTick>(PeriodTick).count();
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
					_State.Handler = &(_State.HandlerA = [this, TC_RC_B, OverflowTargetB, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]
									   {
					if (!--_State.OverflowCount) {
						if (--_State.RepeatLeft) {
							_State.OverflowCount = OverflowTargetB;
							Channel.TC_RC += TC_RC_B;
							_State.Handler = &_State.HandlerB;
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						DoA();
						if (!_State.RepeatLeft)
							DoneCallback();
					} });
					_State.HandlerB = [this, TC_RC_A, OverflowTargetA, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]
					{
						if (!--_State.OverflowCount)
						{
							if (--_State.RepeatLeft)
							{
								_State.OverflowCount = OverflowTargetA;
								Channel.TC_RC += TC_RC_A;
								_State.Handler = &_State.HandlerA;
							}
							else
								Channel.TC_CCR = TC_CCR_CLKDIS;
							DoB();
							if (!_State.RepeatLeft)
								DoneCallback();
						}
					};
					Channel.TC_IER = TC_IER_CPCS;
					return;
				}
				TCCLKS = TC_CMR_TCCLKS_TIMER_CLOCK5;
				Channel.TC_RA = TimeTicksA;
				Channel.TC_RC = PeriodSlow;
			}
			Channel.TC_CMR = TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TCCLKS;
			_State.Handler = &(_State.HandlerA = [this, DoA = std::move(DoA), DoneCallback = std::move(DoneCallback)]()
							   {
			if (--_State.RepeatLeft)
				_State.Handler = &_State.HandlerB;
			else
				Channel.TC_CCR = TC_CCR_CLKDIS;
			DoA();
			if (!_State.RepeatLeft)
				DoneCallback(); });
			_State.HandlerB = [this, DoB = std::move(DoB), DoneCallback = std::move(DoneCallback)]()
			{
				if (--_State.RepeatLeft)
					_State.Handler = &_State.HandlerA;
				else
					Channel.TC_CCR = TC_CCR_CLKDIS;
				DoB();
				if (!_State.RepeatLeft)
					DoneCallback();
			};
			Channel.TC_IER = TC_IER_CPAS | TC_IER_CPCS;
		}
	}
	_PeripheralState _TimerStates[(uint8_t)_PeripheralEnum::NumPeripherals];
}
#endif