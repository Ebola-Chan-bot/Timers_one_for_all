#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_TIMER0
void TimerClass0::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
{
	TIMSK0 = 1 << TOIE0;
	OverflowCountA = _PrescalerOverflow<_Prescaler01>(After.count() >> 8, TCCR0B) + 1;
	TCCR0A = 0;
	OCR0A = After.count() >> _Prescaler01::BitShifts[TCCR0B];
	_COMPA = &(HandlerA = [_TOFA_Capture(Do)]()
			   {
			if (!--HardwareTimer0.OverflowCountA) {
				TIMSK0 = 1 << TOIE0;
				Do();
			} });
	TCNT0 = 0;
	TIFR0 = -1;
	TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
}
void TimerClass0::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (RepeatTimes)
	{
		TIMSK0 = 1 << TOIE0;
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(Every.count() >> 8, TCCR0B))
		{
			// 在有溢出情况下，CTC需要多用一个中断且逻辑更复杂，毫无必要，直接用加法更简明
			const uint8_t OcraTarget = Every.count() >> _Prescaler01::MaxShift;
			TCCR0A = 0;
			OCR0A = OcraTarget;
			OverflowCountA = ++OverflowTarget;
			_COMPA = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback), OverflowTarget, OcraTarget]()
					   {
					if (!--HardwareTimer0.OverflowCountA) {
						if (--HardwareTimer0.RepeatLeft) {
							HardwareTimer0.OverflowCountA = OverflowTarget;
							OCR0A += OcraTarget;
						}
						else
							TIMSK0 = 1 << TOIE0;
						// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
						Do();
						if (!HardwareTimer0.RepeatLeft)
							DoneCallback();
					} });
		}
		else
		{
			TCCR0A = 1 << WGM01;
			OCR0A = Every.count() >> _Prescaler01::BitShifts[TCCR0B];
			_COMPA = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
					   {
					if (!--HardwareTimer0.RepeatLeft)
						TIMSK0 = 1 << TOIE0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!HardwareTimer0.RepeatLeft)
						DoneCallback(); });
		}
		RepeatLeft = RepeatTimes;
		TCNT0 = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
	}
	else
		DoneCallback();
}
void TimerClass0::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (NumHalfPeriods)
	{
		TIMSK0 = 1 << TOIE0;
		AfterB += AfterA;
		_TOFA_PublicCache(DoneCallback);
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(AfterB.count() >> 8, TCCR0B))
		{
			TCCR0A = 0;
			OCR0A = AfterA.count() >> _Prescaler01::MaxShift;
			OverflowCountA = (AfterA.count() >> _Prescaler01::MaxShift + 8) + 1;
			const uint8_t OcrTarget = AfterB.count() >> _Prescaler01::MaxShift;
			_COMPA = &(HandlerA = [OcrTarget, OverflowTarget, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (!--HardwareTimer0.OverflowCountA) {
						if (--HardwareTimer0.RepeatLeft > 1) {
							HardwareTimer0.OverflowCountA = OverflowTarget;
							OCR0A += OcrTarget;
						}
						else
							TIMSK0 &= ~(1 << OCIE0A);
						DoA();
						if (!HardwareTimer0.RepeatLeft)
							_TOFA_SingletonReference(HardwareTimer0, DoneCallback)();
					} });
			OverflowCountB = OverflowTarget;
			OCR0B = OcrTarget;
			_COMPB = &(HandlerB = [OcrTarget, OverflowTarget, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
					   {
							if (!--HardwareTimer0.OverflowCountA) {
								if (--HardwareTimer0.RepeatLeft > 1) {
									HardwareTimer0.OverflowCountB = OverflowTarget;
									OCR0B += OcrTarget;
								}
								else
									TIMSK0 &= ~(1 << OCIE0B);
								DoB();
								if (!HardwareTimer0.RepeatLeft)
									_TOFA_SingletonReference(HardwareTimer0, DoneCallback)();
							} });
		}
		else
		{
			const uint8_t BitShift = _Prescaler01::BitShifts[TCCR0B];
			TCCR0A = 1 << WGM01;
			// AB置反，因为DoB需要仅适用于A的CTC
			OCR0B = AfterA.count() >> BitShift;
			_COMPB = &(HandlerB = [_TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (--HardwareTimer0.RepeatLeft < 2)
						TIMSK0 &= ~(1 << OCIE0B);
					DoA();
					if (!HardwareTimer0.RepeatLeft)
						_TOFA_SingletonReference(HardwareTimer0, DoneCallback)(); });
			OCR0A = AfterB.count() >> BitShift;
			_COMPA = &(HandlerA = [_TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (--HardwareTimer0.RepeatLeft < 2)
						TIMSK0 &= ~(1 << OCIE0A);
					DoB();
					if (!HardwareTimer0.RepeatLeft)
						_TOFA_SingletonReference(HardwareTimer0, DoneCallback)(); });
		}
		RepeatLeft = NumHalfPeriods;
		TCNT0 = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0);
	}
	else
		DoneCallback();
}
#endif
#if defined TOFA_TIMER1 || defined TOFA_TIMER3 || defined TOFA_TIMER4 || defined TOFA_TIMER5
void TimerClass1::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
{
	TIMSK = 0;
	OverflowCountA = _PrescalerOverflow<_Prescaler01>(After.count() >> 16, TCCRB) + 1;
	TCCRA = 0;
	OCRA = After.count() >> _Prescaler01::BitShifts[TCCRB];
	_COMPA = &(HandlerA = [this, _TOFA_Capture(Do)]()
			   {
			if (!--OverflowCountA) {
				TIMSK = 0;
				Do();
			} });
	TCNT = 0;
	TIFR = -1;
	TIMSK = 1 << OCIE1A;
}
void TimerClass1::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (RepeatTimes)
	{
		TIMSK = 0;
		TCCRA = 0;
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(Every.count() >> 16, TCCRB))
		{
			const uint16_t OcraTarget = Every.count() >> _Prescaler01::MaxShift;
			OCRA = OcraTarget;
			OverflowCountA = ++OverflowTarget;
			_COMPA = &(HandlerA = [this, _TOFA_Capture(Do), _TOFA_Capture(DoneCallback), OverflowTarget, OcraTarget]()
					   {
					if (!--OverflowCountA) {
						if (--RepeatLeft) {
							OverflowCountA = OverflowTarget;
							OCRA += OcraTarget;
						}
						else
							TIMSK = 0;
						// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
						Do();
						if (!RepeatLeft)
							DoneCallback();
					} });
		}
		else
		{
			OCRA = Every.count() >> _Prescaler01::BitShifts[TCCRB];
			_COMPA = &(HandlerA = [this, _TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
					   {
					if (!--RepeatLeft)
						TIMSK = 0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!RepeatLeft)
						DoneCallback(); });
			TCCRB |= 1 << WGM12;
		}
		RepeatLeft = RepeatTimes;
		TCNT = 0;
		TIFR = -1;
		TIMSK = 1 << OCIE1A;
	}
	else
		DoneCallback();
}
void TimerClass1::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (NumHalfPeriods)
	{
		TIMSK = 0;
		AfterB += AfterA;
		TCCRA = 0;
		_TOFA_PublicCache(DoneCallback);
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler01>(AfterB.count() >> 16, TCCRB))
		{
			OCRA = AfterA.count() >> _Prescaler01::MaxShift;
			OverflowCountA = (AfterA.count() >> _Prescaler01::MaxShift + 16) + 1;
			const uint16_t OcrTarget = AfterB.count() >> _Prescaler01::MaxShift;
			_COMPA = &(HandlerA = [this, OcrTarget, OverflowTarget, _TOFA_Capture(DoA) _TOFA_ThisCapture(DoneCallback)]()
					   {
					if (!--OverflowCountA) {
						if (--RepeatLeft > 1) {
							OverflowCountA = OverflowTarget;
							OCRA += OcrTarget;
						}
						else
							TIMSK &= ~(1 << OCIE1A);
						DoA();
						if (!RepeatLeft)
							_TOFA_ThisReference(DoneCallback)();
					} });
			OverflowCountB = OverflowTarget;
			OCRB = OcrTarget;
			_COMPB = &(HandlerB = [this, OcrTarget, OverflowTarget, _TOFA_Capture(DoB) _TOFA_ThisCapture(DoneCallback)]()
					   {
							if (!--OverflowCountA) {
								if (--RepeatLeft > 1) {
									OverflowCountB = OverflowTarget;
									OCRB += OcrTarget;
								}
								else
									TIMSK &= ~(1 << OCIE1B);
								DoB();
								if (!RepeatLeft)
									_TOFA_ThisReference(DoneCallback)();
							} });
		}
		else
		{
			const uint8_t BitShift = _Prescaler01::BitShifts[TCCRB];
			// AB置反，因为DoB需要仅适用于A的CTC
			OCRB = AfterA.count() >> BitShift;
			_COMPB = &(HandlerB = [this, _TOFA_Capture(DoA) _TOFA_ThisCapture(DoneCallback)]()
					   {
					if (--RepeatLeft < 2)
						TIMSK &= ~(1 << OCIE1B);
					DoA();
					if (!RepeatLeft)
						_TOFA_ThisReference(DoneCallback)(); });
			OCRA = AfterB.count() >> BitShift;
			_COMPA = &(HandlerA = [this, _TOFA_Capture(DoB) _TOFA_ThisCapture(DoneCallback)]()
					   {
					if (--RepeatLeft < 2)
						TIMSK &= ~(1 << OCIE1A);
					DoB();
					if (!RepeatLeft)
						_TOFA_ThisReference(DoneCallback)(); });
			TCCRB |= 1 << WGM12;
		}
		RepeatLeft = NumHalfPeriods;
		TCNT = 0;
		TIFR = -1;
		TIMSK = (1 << OCIE1A) | (1 << OCIE1B);
	}
	else
		DoneCallback();
}
#endif
#ifdef TOFA_TIMER2
void TimerClass2::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
{
	TIMSK2 = 0;
	OverflowCountA = _PrescalerOverflow<_Prescaler2>(After.count() >> 8, TCCR2B) + 1;
	TCCR2A = 0;
	OCR2A = After.count() >> _Prescaler2::BitShifts[TCCR2B];
	_COMPA = &(HandlerA = [_TOFA_Capture(Do)]()
			   {
			if (!--HardwareTimer2.OverflowCountA) {
				TIMSK2 = 0;
				Do();
			} });
	TCNT2 = 0;
	TIFR2 = -1;
	TIMSK2 = 1 << OCIE2A;
}
void TimerClass2::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (RepeatTimes)
	{
		TIMSK2 = 0;
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler2>(Every.count() >> 8, TCCR2B))
		{
			const uint8_t OcraTarget = Every.count() >> _Prescaler2::MaxShift;
			TCCR2A = 0;
			OCR2A = OcraTarget;
			OverflowCountA = ++OverflowTarget;
			_COMPA = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback), OverflowTarget, OcraTarget]()
					   {
					if (!--HardwareTimer2.OverflowCountA) {
						if (--HardwareTimer2.RepeatLeft) {
							HardwareTimer2.OverflowCountA = OverflowTarget;
							OCR2A += OcraTarget;
						}
						else
							TIMSK2 = 0;
						// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
						Do();
						if (!HardwareTimer2.RepeatLeft)
							DoneCallback();
					} });
		}
		else
		{
			TCCR2A = 1 << WGM21;
			OCR2A = Every.count() >> _Prescaler2::BitShifts[TCCR2B];
			_COMPA = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
					   {
					if (!--HardwareTimer2.RepeatLeft)
						TIMSK2 = 0;
					// 计时器配置必须放在前面，然后才能执行动作。因为用户自定义动作可能包含计时器配置，如果放前面会被覆盖掉。
					Do();
					if (!HardwareTimer2.RepeatLeft)
						DoneCallback(); });
		}
		RepeatLeft = RepeatTimes;
		TCNT2 = 0;
		TIFR2 = -1;
		TIMSK2 = 1 << OCIE2A;
	}
	else
		DoneCallback();
}
void TimerClass2::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (NumHalfPeriods)
	{
		TIMSK2 = 0;
		AfterB += AfterA;
		_TOFA_PublicCache(DoneCallback);
		if (uint32_t OverflowTarget = _PrescalerOverflow<_Prescaler2>(AfterB.count() >> 8, TCCR2B))
		{
			TCCR2A = 0;
			OCR2A = AfterA.count() >> _Prescaler2::MaxShift;
			OverflowCountA = (AfterA.count() >> _Prescaler2::MaxShift + 8) + 1;
			const uint8_t OcrTarget = AfterB.count() >> _Prescaler2::MaxShift;
			_COMPA = &(HandlerA = [OcrTarget, OverflowTarget, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (!--HardwareTimer2.OverflowCountA) {
						if (--HardwareTimer2.RepeatLeft > 1) {
							HardwareTimer2.OverflowCountA = OverflowTarget;
							OCR2A += OcrTarget;
						}
						else
							TIMSK2 &= ~(1 << OCIE2A);
						DoA();
						if (!HardwareTimer2.RepeatLeft)
							_TOFA_SingletonReference(HardwareTimer2, DoneCallback)();
					} });
			OverflowCountB = OverflowTarget;
			OCR2B = OcrTarget;
			_COMPB = &(HandlerB = [OcrTarget, OverflowTarget, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
					   {
							if (!--HardwareTimer2.OverflowCountA) {
								if (--HardwareTimer2.RepeatLeft > 1) {
									HardwareTimer2.OverflowCountB = OverflowTarget;
									OCR2B += OcrTarget;
								}
								else
									TIMSK2 &= ~(1 << OCIE2B);
								DoB();
								if (!HardwareTimer2.RepeatLeft)
									_TOFA_SingletonReference(HardwareTimer2, DoneCallback)();
							} });
		}
		else
		{
			const uint8_t BitShift = _Prescaler2::BitShifts[TCCR2B];
			TCCR2A = 1 << WGM21;
			// AB置反，因为DoB需要仅适用于A的CTC
			OCR2B = AfterA.count() >> BitShift;
			_COMPB = &(HandlerB = [_TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (--HardwareTimer2.RepeatLeft < 2)
						TIMSK2 &= ~(1 << OCIE2B);
					DoA();
					if (!HardwareTimer2.RepeatLeft)
						_TOFA_SingletonReference(HardwareTimer2, DoneCallback)(); });
			OCR2A = AfterB.count() >> BitShift;
			_COMPA = &(HandlerA = [_TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
					   {
					if (--HardwareTimer2.RepeatLeft < 2)
						TIMSK2 &= ~(1 << OCIE2A);
					DoB();
					if (!HardwareTimer2.RepeatLeft)
						_TOFA_SingletonReference(HardwareTimer2, DoneCallback)(); });
		}
		RepeatLeft = NumHalfPeriods;
		TCNT2 = 0;
		TIFR2 = -1;
		TIMSK2 = (1 << OCIE2A) | (1 << OCIE2B);
	}
	else
		DoneCallback();
}
#endif
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
void SystemTimerClass::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
{
	SysTick->CTRL = 0;
	SysTick->VAL = 0;
	uint64_t TimeTicks = After.count();
	uint32_t CTRL = _SysTick_CTRL_MCK;
	HandlerA = [_TOFA_Capture(Do)]()
	{
		SysTick->CTRL = 0;
		Do();
	};
	if (TimeTicks >> 24)
	{
		TimeTicks >>= 3;
		if (OverflowCount = TimeTicks >> 24)
		{
			_Handler = &(HandlerB = []()
						 {
					if (!--SystemTimer.OverflowCount)
						SystemTimer._Handler = &SystemTimer.HandlerA; });
			SysTick->LOAD = TimeTicks;
			SysTick->CTRL = _SysTick_CTRL_MCK8;
			while (!SysTick->VAL)
				;
			SysTick->LOAD = -1;
			return;
		}
		CTRL = _SysTick_CTRL_MCK8;
	}
	_Handler = &HandlerA;
	SysTick->LOAD = TimeTicks;
	SysTick->CTRL = CTRL;
}
void SystemTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (RepeatTimes)
	{
		SysTick->CTRL = 0;
		SysTick->VAL = 0;
		uint64_t TimeTicks = Every.count();
		uint32_t CTRL = _SysTick_CTRL_MCK;
		RepeatLeft = RepeatTimes;
		if (TimeTicks >> 24)
		{
			TimeTicks >>= 3;
			if (uint32_t OverflowTarget = TimeTicks >> 24)
			{
				const uint32_t LOAD = TimeTicks;
				OverflowTarget++;
				OverflowCount = OverflowTarget;
				_Handler = &(HandlerA = [_TOFA_Capture(Do), OverflowTarget, LOAD, _TOFA_Capture(DoneCallback)]()
							 {
						switch (--SystemTimer.OverflowCount) {
						case 0:
							if (--SystemTimer.RepeatLeft) {
								SysTick->LOAD = -1;
								SystemTimer.OverflowCount = OverflowTarget;
							}
							else
								SysTick->CTRL = 0;
							Do();
							if (!SystemTimer.RepeatLeft)
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
		_Handler = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
					 {
				if (!--SystemTimer.RepeatLeft)
					SysTick->CTRL = 0;
				Do();
				if (!SystemTimer.RepeatLeft)
					DoneCallback(); });
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
	}
	else
		DoneCallback();
}
void SystemTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (NumHalfPeriods)
	{
		SysTick->CTRL = 0;
		SysTick->VAL = 0;
		uint64_t TimeTicksA = AfterA.count();
		uint64_t TimeTicksB = AfterB.count();
		uint32_t CTRL = _SysTick_CTRL_MCK;
		SysTick->LOAD = -1;
		_TOFA_PublicCache(DoneCallback);
		if (std::max(TimeTicksA, TimeTicksB) >> 24)
		{
			TimeTicksA >>= 3;
			TimeTicksB >>= 3;
			uint32_t OverflowTargetA = (TimeTicksA >> 24);
			uint32_t OverflowTargetB = (TimeTicksB >> 24);
			if (std::max(OverflowTargetA, OverflowTargetB))
			{
				const uint32_t TicksLeftA = TimeTicksA;
				const uint32_t TicksLeftB = TimeTicksB;
				if (OverflowTargetA)
				{
					OverflowCount = ++OverflowTargetA;
					if (OverflowTargetB)
					{
						OverflowTargetB++;
						HandlerA = [TicksLeftB, OverflowTargetB, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
						{
							switch (--SystemTimer.OverflowCount)
							{
							case 0:
								if (--SystemTimer.RepeatLeft)
								{
									SysTick->LOAD = -1;
									SystemTimer.OverflowCount = OverflowTargetB;
									SystemTimer._Handler = &SystemTimer.HandlerB;
								}
								else
									SysTick->CTRL = 0;
								DoA();
								if (!SystemTimer.RepeatLeft)
									_TOFA_SingletonReference(SystemTimer, DoneCallback)();
								break;
							case 1:
								SysTick->LOAD = TicksLeftB;
							}
						};
						HandlerB = [TicksLeftA, OverflowTargetA, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
						{
							switch (--SystemTimer.OverflowCount)
							{
							case 0:
								if (--SystemTimer.RepeatLeft)
								{
									SysTick->LOAD = -1;
									SystemTimer.OverflowCount = OverflowTargetA;
									SystemTimer._Handler = &SystemTimer.HandlerA;
								}
								else
									SysTick->CTRL = 0;
								DoB();
								if (!SystemTimer.RepeatLeft)
									_TOFA_SingletonReference(SystemTimer, DoneCallback)();
								break;
							case 1:
								SysTick->LOAD = TicksLeftA;
							}
						};
					}
					else
					{
						HandlerA = [TicksLeftB, TicksLeftA, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
						{
							switch (--SystemTimer.OverflowCount)
							{
							case 0:
								if (--SystemTimer.RepeatLeft)
								{
									SysTick->LOAD = TicksLeftA;
									SystemTimer._Handler = &SystemTimer.HandlerB;
								}
								else
									SysTick->CTRL = 0;
								DoA();
								if (!SystemTimer.RepeatLeft)
									_TOFA_SingletonReference(SystemTimer, DoneCallback)();
								break;
							case 1:
								SysTick->LOAD = TicksLeftB;
							}
						};
						HandlerB = [OverflowTargetA, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
						{
							if (--SystemTimer.RepeatLeft)
							{
								SysTick->LOAD = -1;
								SystemTimer.OverflowCount = OverflowTargetA;
								SystemTimer._Handler = &SystemTimer.HandlerA;
							}
							else
								SysTick->CTRL = 0;
							DoB();
							if (!SystemTimer.RepeatLeft)
								_TOFA_SingletonReference(SystemTimer, DoneCallback)();
						};
					}
				}
				else
				{
					HandlerA = [OverflowTargetB, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					{
						if (--SystemTimer.RepeatLeft)
						{
							SysTick->LOAD = -1;
							SystemTimer.OverflowCount = OverflowTargetB;
							SystemTimer._Handler = &SystemTimer.HandlerB;
						}
						else
							SysTick->CTRL = 0;
						DoA();
						if (!SystemTimer.RepeatLeft)
							_TOFA_SingletonReference(SystemTimer, DoneCallback)();
					};
					HandlerB = [TicksLeftA, TicksLeftB, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
					{
						switch (--SystemTimer.OverflowCount)
						{
						case 0:
							if (--SystemTimer.RepeatLeft)
							{
								SysTick->LOAD = TicksLeftB;
								SystemTimer._Handler = &SystemTimer.HandlerA;
							}
							else
								SysTick->CTRL = 0;
							DoB();
							if (!SystemTimer.RepeatLeft)
								_TOFA_SingletonReference(SystemTimer, DoneCallback)();
							break;
						case 1:
							SysTick->LOAD = TicksLeftA;
						}
					};
				}
				_Handler = &HandlerA;
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
		_Handler = &(HandlerA = [TicksLeftA, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					 {
				if (--SystemTimer.RepeatLeft) {
					SysTick->LOAD = TicksLeftA;
					SystemTimer._Handler = &SystemTimer.HandlerB;
				}
				else
					SysTick->CTRL = 0;
				DoA();
				if (!SystemTimer.RepeatLeft)
					_TOFA_SingletonReference(SystemTimer, DoneCallback)(); });
		HandlerB = [TicksLeftB, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
		{
			if (--SystemTimer.RepeatLeft)
			{
				SysTick->LOAD = TicksLeftB;
				SystemTimer._Handler = &SystemTimer.HandlerA;
			}
			else
				SysTick->CTRL = 0;
			DoB();
			if (!SystemTimer.RepeatLeft)
				_TOFA_SingletonReference(SystemTimer, DoneCallback)();
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
#endif
#ifdef TOFA_REALTIMER
void RealTimerClass::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
{
	RTT->RTT_MR = 0;
	HandlerA = [_TOFA_Capture(Do)]()
	{
		RealTimer._Handler = nullptr;
		RTT->RTT_MR = 0;
		Do();
	};
	const uint64_t TimerTicks = std::chrono::duration_cast<_SlowTick>(After).count();
	while (RTT->RTT_SR)
		;
	NVIC_EnableIRQ(RTT_IRQn);
	if (const uint32_t RTPRES = TimerTicks >> 32)
	{
		if (OverflowCount = RTPRES >> 16)
		{
			RTT->RTT_AR = TimerTicks >> 16;
			_Handler = &(HandlerB = []()
						 {
					if (!--RealTimer.OverflowCount)
						RealTimer.HandlerA(); });
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
	_Handler = &HandlerA;
}
void RealTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
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
				OverflowCount = ++OverflowTarget;
				_Handler = &(HandlerA = [_TOFA_Capture(Do), RTT_AR, OverflowTarget, _TOFA_Capture(DoneCallback)]()
							 {
						if (!--RealTimer.OverflowCount) {
							if (--RealTimer.RepeatLeft) {
								RTT->RTT_AR = RTT_AR; // 伪暂停算法可能修改AR，所以每次都要重新设置
								RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
								RealTimer.OverflowCount = OverflowTarget;
							}
							else {
								RTT->RTT_MR = 0;
								RealTimer._Handler = nullptr;
							}
							Do();
							if (!RealTimer.RepeatLeft)
								DoneCallback();
						} });
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
			}
			else
			{
				RTT->RTT_AR = -1;
				RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | RTPRES;
				_Handler = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
							 {
						if (!--RealTimer.RepeatLeft) {
							RTT->RTT_MR = 0;
							RealTimer._Handler = nullptr;
						}
						Do();
						if (!RealTimer.RepeatLeft)
							DoneCallback(); });
			}
		}
		else
		{
			const uint32_t RTT_AR = TimerTicks;
			RTT->RTT_AR = RTT_AR;
			_Handler = &(HandlerA = [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback), RTT_AR]()
						 {
					if (--RealTimer.RepeatLeft)
						RTT->RTT_AR += RTT_AR;
					else {
						RTT->RTT_MR = 0;
						RealTimer._Handler = nullptr;
					}
					Do();
					if (!RealTimer.RepeatLeft)
						DoneCallback(); });
			RTT->RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST | 1;
		}
		RepeatLeft = RepeatTimes;
		while (RTT->RTT_SR)
			;
		NVIC_EnableIRQ(RTT_IRQn);
	}
	else
		DoneCallback();
}
void RealTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	if (NumHalfPeriods)
	{
		RTT->RTT_MR = 0;
		const uint64_t TimerTicksA = std::chrono::duration_cast<_SlowTick>(AfterA).count();
		const uint64_t TimerTicksB = std::chrono::duration_cast<_SlowTick>(AfterB).count();
		const uint64_t MaxTicks = std::max(TimerTicksA, TimerTicksB);
		RepeatLeft = NumHalfPeriods;
		uint32_t RTPRES = MaxTicks >> 32;
		uint32_t RTT_AR_A;
		uint32_t RTT_AR_B;
		uint32_t RTT_MR;
		while (RTT->RTT_SR)
			;
		NVIC_EnableIRQ(RTT_IRQn);
		_TOFA_PublicCache(DoneCallback);
		if (RTPRES)
		{
			if (RTPRES >> 16)
			{
				RTT_AR_A = TimerTicksA >> 16;
				RTT->RTT_AR = RTT_AR_A;
				RTT_MR = RTT_MR_ALMIEN | RTT_MR_RTTRST;
				const uint16_t OverflowTargetA = (TimerTicksA >> 48) + 1;
				OverflowCount = OverflowTargetA;
				const uint16_t OverflowTargetB = (TimerTicksB >> 48) + 1;
				RTT_AR_B = TimerTicksB >> 16;
				_Handler = &(HandlerA = [OverflowTargetB, RTT_AR_B, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
							 {
						if (!--RealTimer.OverflowCount) {
							if (--RealTimer.RepeatLeft) {
								RealTimer.OverflowCount += OverflowTargetB;
								RTT->RTT_AR += RTT_AR_B;
								RealTimer._Handler = &RealTimer.HandlerB;
							}
							else {
								RTT->RTT_MR = 0;
								RealTimer._Handler = nullptr;
							}
							DoA();
							if (!RealTimer.RepeatLeft)
								_TOFA_SingletonReference(RealTimer, DoneCallback)();
						} });
				HandlerB = [OverflowTargetA, RTT_AR_A, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
				{
					if (!--RealTimer.OverflowCount)
					{
						if (--RealTimer.RepeatLeft)
						{
							RealTimer.OverflowCount += OverflowTargetA;
							RTT->RTT_AR += RTT_AR_A;
							RealTimer._Handler = &RealTimer.HandlerA;
						}
						else
						{
							RTT->RTT_MR = 0;
							RealTimer._Handler = nullptr;
						}
						DoB();
						if (!RealTimer.RepeatLeft)
							_TOFA_SingletonReference(RealTimer, DoneCallback)();
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
		_Handler = &(HandlerA = [RTT_AR_B, _TOFA_Capture(DoA) _TOFA_SingletonCapture(DoneCallback)]()
					 {
				if (--RealTimer.RepeatLeft) {
					RTT->RTT_AR += RTT_AR_B;
					RealTimer._Handler = &RealTimer.HandlerB;
				}
				else {
					RTT->RTT_MR = 0;
					RealTimer._Handler = nullptr;
				}
				DoA();
				if (!RealTimer.RepeatLeft)
					_TOFA_SingletonReference(RealTimer, DoneCallback)(); });
		HandlerB = [RTT_AR_A, _TOFA_Capture(DoB) _TOFA_SingletonCapture(DoneCallback)]()
		{
			if (--RealTimer.RepeatLeft)
			{
				RTT->RTT_AR += RTT_AR_A;
				RealTimer._Handler = &RealTimer.HandlerA;
			}
			else
			{
				RTT->RTT_MR = 0;
				RealTimer._Handler = nullptr;
			}
			DoB();
			if (!RealTimer.RepeatLeft)
				_TOFA_SingletonReference(RealTimer, DoneCallback)();
		};
		RTT->RTT_MR = RTT_MR;
	}
	else
		DoneCallback();
}
#endif
void PeripheralTimerClass::DoAfter(Tick After, std::move_only_function<void() const> _TOFA_Reference Do)
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
		if (OverflowCount = SlowTicks >> 32)
		{
			Channel.TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
			_Handler = &(HandlerA = [this, _TOFA_Capture(Do)]
						 {
					// OverflowCount比实际所需次数少一次，正好利用这一点提前启动CPCDIS
					if (!--OverflowCount) {
						Channel.TC_CMR |= TC_CMR_CPCDIS;
						_Handler = &Do;
					} });
		}
	}
	Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_CPCDIS | TC_CMR_WAVE | TCCLKS;
	_Handler = _TOFA_DirectHandler;
	Channel.TC_IER = TC_IER_CPCS;
}
void PeripheralTimerClass::RepeatEvery(Tick Every, std::move_only_function<void() const> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	switch (RepeatTimes)
	{
	case 0:
		DoneCallback();
		break;
	case 1:
		// 重复1次需要特殊处理，因为会导致CPCDIS不可用
		DoAfter(Every, [_TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]()
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
		RepeatLeft = RepeatTimes;
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
				_Handler = &(HandlerA = [this, TC_RC, OverflowTarget, _TOFA_Capture(Do), _TOFA_Capture(DoneCallback)]
							 {
						if (!--OverflowCount) {
							if (--RepeatLeft) {
								OverflowCount = OverflowTarget;
								Channel.TC_RC += TC_RC;
							}
							else
								Channel.TC_CCR = TC_CCR_CLKDIS;
							Do();
							if (!RepeatLeft)
								DoneCallback();
						} });
				Channel.TC_IER = TC_IER_CPCS;
				return;
			}
		}
		Channel.TC_CMR = TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TCCLKS;
		_TOFA_PublicCache(Do);
		_Handler = &(HandlerA = [this _TOFA_ThisCapture(Do)]()
					 {
				if (--RepeatLeft == 1) {
					Channel.TC_CMR |= TC_CMR_CPCDIS;
					_Handler = &HandlerB;
				}
				_TOFA_ThisReference(Do)(); });
		HandlerB = [_TOFA_Capture(DoneCallback) _TOFA_SelfCapture(Do)]()
		{
			Do();
			DoneCallback();
		};
		Channel.TC_IER = TC_IER_CPCS;
	}
}
void PeripheralTimerClass::DoubleRepeat(Tick AfterA, std::move_only_function<void() const> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void() const> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> _TOFA_Reference DoneCallback)
{
	switch (NumHalfPeriods)
	{
	case 0:
		DoneCallback();
		break;
	case 1:
		// 重复1次需要特殊处理，因为会导致CPCDIS不可用
		DoAfter(AfterA, [_TOFA_Capture(DoA), _TOFA_Capture(DoneCallback)]()
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
		RepeatLeft = NumHalfPeriods;
		_TOFA_PublicCache(DoneCallback);
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
				OverflowCount = OverflowTargetA;
				Channel.TC_RC = TC_RC_A;
				const uint32_t OverflowTargetB = TimeTicksB >> 32;
				const uint32_t TC_RC_B = TimeTicksB;
				_Handler = &(HandlerA = [this, TC_RC_B, OverflowTargetB, _TOFA_Capture(DoA) _TOFA_ThisCapture(DoneCallback)]
							 {
						if (!--OverflowCount) {
							if (--RepeatLeft) {
								OverflowCount = OverflowTargetB;
								Channel.TC_RC += TC_RC_B;
								_Handler = &HandlerB;
							}
							else
								Channel.TC_CCR = TC_CCR_CLKDIS;
							DoA();
							if (!RepeatLeft)
								_TOFA_ThisReference(DoneCallback)();
						} });
				HandlerB = [this, TC_RC_A, OverflowTargetA, _TOFA_Capture(DoB) _TOFA_ThisCapture(DoneCallback)]
				{
					if (!--OverflowCount)
					{
						if (--RepeatLeft)
						{
							OverflowCount = OverflowTargetA;
							Channel.TC_RC += TC_RC_A;
							_Handler = &HandlerA;
						}
						else
							Channel.TC_CCR = TC_CCR_CLKDIS;
						DoB();
						if (!RepeatLeft)
							_TOFA_ThisReference(DoneCallback)();
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
		_Handler = &(HandlerA = [this, _TOFA_Capture(DoA) _TOFA_ThisCapture(DoneCallback)]()
					 {
				if (--RepeatLeft)
					_Handler = &HandlerB;
				else
					Channel.TC_CCR = TC_CCR_CLKDIS;
				DoA();
				if (!RepeatLeft)
					_TOFA_ThisReference(DoneCallback)(); });
		HandlerB = [this, _TOFA_Capture(DoB) _TOFA_ThisCapture(DoneCallback)]()
		{
			if (--RepeatLeft)
				_Handler = &HandlerA;
			else
				Channel.TC_CCR = TC_CCR_CLKDIS;
			DoB();
			if (!RepeatLeft)
				_TOFA_ThisReference(DoneCallback)();
		};
		Channel.TC_IER = TC_IER_CPAS | TC_IER_CPCS;
	}
}
#endif