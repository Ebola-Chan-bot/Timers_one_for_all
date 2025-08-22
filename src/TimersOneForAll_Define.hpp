#pragma once
#ifdef _TOFA_DEBUG
#include "TimersOneForAll_Declare.hpp"
#endif
namespace Timers_one_for_all
{
	std::move_only_function<void()> _DoNothing;
	TimerClass *AllocateTimer()
	{
		for (int8_t T = NumTimers - 1; T >= 0; --T)
		{
			TimerClass *const HT = HardwareTimers[T];
			if (HT->Allocatable)
			{
				HT->Allocatable = false;
				return HT;
			}
		}
		return nullptr;
	}
#ifdef ARDUINO_ARCH_AVR
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
	// 输入预分频器应排除1号的不0右移分频器。TrailingZeros表示周期数的log2向下取整，因此0时已经超过一周期，需要从2号开始
	template <typename T, uint8_t TrailingZeros = 0, uint8_t PrescalerIndex = 2>
	struct _BitLimit
	{
		using type = typename _U8SequencePrepend<PrescalerIndex, typename _BitLimit<T, TrailingZeros + 1, PrescalerIndex>::type>::type;
	};
	template <uint8_t TrailingZeros, uint8_t PrescalerIndex, uint8_t... Rest>
	struct _BitLimit<_U8Sequence<TrailingZeros + 1, Rest...>, TrailingZeros, PrescalerIndex>
	{
		using type = typename _U8SequencePrepend<PrescalerIndex, typename _BitLimit<_U8Sequence<Rest...>, TrailingZeros + 1, PrescalerIndex + 1>::type>::type;
	};
	template <uint8_t TrailingZeros, uint8_t PrescalerIndex>
	struct _BitLimit<_U8Sequence<TrailingZeros + 1>, TrailingZeros, PrescalerIndex>
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
#define _TOFA_TimerIsr(T)             \
	ISR(TIMER##T##_OVF_vect)          \
	{                                 \
		(*HardwareTimer##T._OVF)();   \
	}                                 \
	ISR(TIMER##T##_COMPA_vect)        \
	{                                 \
		(*HardwareTimer##T._COMPA)(); \
	}                                 \
	ISR(TIMER##T##_COMPB_vect)        \
	{                                 \
		(*HardwareTimer##T._COMPB)(); \
	}
#ifdef TOFA_TIMER1
	_TOFA_TimerIsr(1);
	TimerClass1 HardwareTimer1{TCCR1A, TCCR1B, TIMSK1, TIFR1, TCNT1, OCR1A, OCR1B};
#endif
#ifdef TOFA_TIMER3
	_TOFA_TimerIsr(3);
	TimerClass1 HardwareTimer3{TCCR3A, TCCR3B, TIMSK3, TIFR3, TCNT3, OCR3A, OCR3B};
#endif
#ifdef TOFA_TIMER4
	_TOFA_TimerIsr(4);
	TimerClass1 HardwareTimer4{TCCR4A, TCCR4B, TIMSK4, TIFR4, TCNT4, OCR4A, OCR4B};
#endif
#ifdef TOFA_TIMER5
	_TOFA_TimerIsr(5);
	TimerClass1 HardwareTimer5{TCCR5A, TCCR5B, TIMSK5, TIFR5, TCNT5, OCR5A, OCR5B};
#endif
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
	uint32_t _PrescalerOverflow(uint32_t Cycles, uint8_t volatile &Clock)
	{
		if (const uint32_t OverflowTarget = Cycles >> Prescaler::MaxShift)
		{
			Clock = Prescaler::MaxClock;
			return OverflowTarget;
		}
		else
		{
			// 如果Cycles大于0，表示超过一周期需要预分频，计算Cycles的log2向下取整以转换为预分频器编号。预分频器编号从1开始，而1号不分频，所以只有Cycles为0的情况取1，其它情况至少是2
			Clock = Cycles ? Prescaler::BitsToPrescaler[sizeof(_FirstArgumentType_t<decltype(__builtin_clz)>) * 8 - 1 - __builtin_clz(Cycles)] : 1;
			return 0;
		}
	}
#ifdef TOFA_TIMER0
#warning "使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER0"
	ISR(TIMER0_COMPA_vect)
	{
		(*HardwareTimer0._COMPA)();
	}
	ISR(TIMER0_COMPB_vect)
	{
		(*HardwareTimer0._COMPB)();
	}
	TimerClass0 HardwareTimer0{TCCR0B};
	bool TimerClass0::Busy() const
	{
		return TIMSK0 & ~(1 << TOIE0);
	}
	void TimerClass0::Stop() const
	{
		TIMSK0 = 1 << TOIE0;
	}
	void TimerClass0::StartTiming()
	{
		TIMSK0 = 1 << TOIE0;
		TCCR0B = 1;
		TCCR0A = 0;
		OverflowCountA = 0;
		_COMPA = &(HandlerA = [this]()
				   {
				if (++OverflowCountA == _Prescaler01::AdvanceFactor[TCCR0B]) {
					OverflowCountA = 1;
					if ((++TCCR0B >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
						_COMPA = &HandlerB;
				} });
		HandlerB = [&OverflowCount = OverflowCountA]()
		{ OverflowCount++; };
		OCR0A = 0;
		TCNT0 = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << OCIE0A) | (1 << TOIE0);
	}
	void TimerClass0::RefreshTiming() const
	{
		uint8_t const State = TIMSK0 & TIFR0;
		TIFR0 = State;
		if (State & 1 << OCF0A)
			(*_COMPA)();
		if (State & 1 << OCF0B)
			(*_COMPB)();
	}
	Tick TimerClass0::_GetTiming() const
	{
		const uint8_t Clock = TCCR0B;
		return Tick(((uint64_t)OverflowCountA << 8) + TCNT0 << _Prescaler01::BitShifts[Clock ? Clock : Clock]);
	}
	void TimerClass0::Delay(Tick Time)
	{
		TIMSK0 = 1 << TOIE0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler01>(Time.count() >> 8, TCCR0B) + 1;
		TCCR0A = 0; // 必须先确保TCCRA低2位全0，否则OCRA会受到限制
		OCR0A = Time.count() >> _Prescaler01::BitShifts[TCCR0B];
		_COMPA = &(HandlerA = [&OverflowCount]()
				   {
					   if (!--OverflowCount)
						   TIMSK0 = 1 << TOIE0; // 由中断负责停止计时器，避免OverflowCount下溢
				   });
		TCNT0 = 0;
		TIFR0 = -1;
		TIMSK0 = (1 << TOIE0) | (1 << OCIE0A);
		while (OverflowCount)
			;
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
	void TimerClass1::StartTiming()
	{
		TIMSK = 0;
		TCCRB = 1;
		TCCRA = 0;
		OverflowCountA = 0;
		_OVF = &(HandlerA = [this]()
				 {
				if (++OverflowCountA == _Prescaler01::AdvanceFactor[TCCRB]) {
					OverflowCountA = 1;
					if ((++TCCRB >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
						_OVF = &HandlerB;
				} });
		HandlerB = [&OverflowCount = OverflowCountA]()
		{ OverflowCount++; };
		TCNT = 0;
		TIFR = -1;
		TIMSK = 1 << TOIE1;
	}
	void TimerClass1::RefreshTiming() const
	{
		uint8_t const State = TIMSK & TIFR;
		TIFR = State;
		if (State & 1 << TOV1)
			(*_OVF)();
		if (State & 1 << OCF1A)
			(*_COMPA)();
		if (State & 1 << OCF1B)
			(*_COMPB)();
	}
	Tick TimerClass1::_GetTiming() const
	{
		const uint8_t Clock = TCCRB;
		return Tick(((uint64_t)OverflowCountA << 16) + TCNT << _Prescaler01::BitShifts[Clock ? Clock : Clock]);
	}
	void TimerClass1::Delay(Tick Time)
	{
		TIMSK = 0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler01>(Time.count() >> 16, TCCRB) + 1;
		TCCRA = 0;
		OCRA = Time.count() >> _Prescaler01::BitShifts[TCCRB];
		_COMPA = &(HandlerA = [&TIMSK = TIMSK, &OverflowCount]()
				   {
				if (!--OverflowCount)
					TIMSK = 0; });
		TCNT = 0;
		TIFR = -1;
		TIMSK = 1 << OCIE1A;
		while (OverflowCount)
			;
	}
#endif
#ifdef TOFA_TIMER2
#warning "使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_TIMER2"
	_TOFA_TimerIsr(2);
	TimerClass2 HardwareTimer2{TCCR2B};
	bool TimerClass2::Busy() const
	{
		return TIMSK2;
	}
	void TimerClass2::Stop() const
	{
		TIMSK2 = 0;
	}
	void TimerClass2::StartTiming()
	{
		TIMSK2 = 0;
		TCCR2B = 1;
		TCCR2A = 0;
		OverflowCountA = 0;
		_OVF = &(HandlerA = [this]()
				 {
				if (++OverflowCountA == _Prescaler01::AdvanceFactor[TCCR2B]) {
					OverflowCountA = 1;
					if ((++TCCR2B >= std::extent_v<decltype(_Prescaler01::AdvanceFactor)>))
						_OVF = &HandlerB;
				} });
		HandlerB = [&OverflowCount = OverflowCountA]()
		{ OverflowCount++; };
		TCNT2 = 0;
		TIFR2 = -1;
		TIMSK2 = 1 << TOIE1;
	}
	void TimerClass2::RefreshTiming() const
	{
		uint8_t const State = TIMSK2 & TIFR2;
		TIFR2 = State;
		if (State & 1 << TOV2)
			(*_OVF)();
		if (State & 1 << OCF2A)
			(*_COMPA)();
		if (State & 1 << OCF2B)
			(*_COMPB)();
	}
	Tick TimerClass2::_GetTiming() const
	{
		const uint8_t Clock = TCCR2B;
		return Tick(((uint64_t)OverflowCountA << 8) + TCNT2 << _Prescaler2::BitShifts[Clock ? Clock : Clock]);
	}
	void TimerClass2::Delay(Tick Time)
	{
		TIMSK2 = 0;
		volatile uint32_t OverflowCount = _PrescalerOverflow<_Prescaler2>(Time.count() >> 8, TCCR2B) + 1;
		TCCR2A = 0;
		OCR2A = Time.count() >> _Prescaler2::BitShifts[TCCR2B];
		_COMPA = &(HandlerA = [&OverflowCount]()
				   {
				if (!--OverflowCount)
					TIMSK2 = 0; });
		TCNT2 = 0;
		TIFR2 = -1;
		TIMSK2 = 1 << OCIE2A;
		while (OverflowCount)
			;
	}
#endif
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
	SystemTimerClass SystemTimer;
	// 系统计时器实测不符合文档描述。设置 CTRL ENABLE 不能将LOAD载入VAL，只能手动设置VAL，但只能将其设为0，设为0以外的任何值都会自动变成0。用这种方式使得VAL变为0时，不会触发中断。VAL载入LOAD需要不确定的时间，需要循环等待VAL已载入后才能修改LOAD，否则旧LOAD可能会被丢弃。这个载入时间与CLKSOURCE无关。VAL载入LOAD发生在中断之前，所以在中断中只能设置下次LOAD值
	extern "C" int sysTickHook()
	{
		if (SystemTimer._Handler)
			(*SystemTimer._Handler)();
		return 0;
	}
	bool SystemTimerClass::Busy() const
	{
		return SysTick->CTRL & SysTick_CTRL_TICKINT_Msk;
	}
	void SystemTimerClass::Pause()
	{
		if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
			SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	}
	void SystemTimerClass::Continue() const
	{
		if ((SysTick->CTRL & (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk)) == SysTick_CTRL_TICKINT_Msk)
			SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}
	void SystemTimerClass::Stop()
	{
		SysTick->CTRL = 0; // 不能简单将Handler设为nullptr，可能陷入无限中断
	}
	constexpr uint32_t _SysTick_CTRL_MCK8 = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	constexpr uint32_t _SysTick_CTRL_MCK = SysTick_CTRL_CLKSOURCE_Msk | _SysTick_CTRL_MCK8;
	void SystemTimerClass::StartTiming()
	{
		SysTick->VAL = 0;
		SysTick->LOAD = -1;
		SysTick->CTRL = _SysTick_CTRL_MCK;
		OverflowCount = 8;
		HandlerB = []()
		{ SystemTimer.OverflowCount++; };
		_Handler = &(HandlerA = []()
					 {
				if (!--SystemTimer.OverflowCount) {
					SysTick->CTRL = _SysTick_CTRL_MCK8;
					SystemTimer.OverflowCount = 1;
					SystemTimer._Handler = &SystemTimer.HandlerB;
				} });
	}
	void SystemTimerClass::RefreshTiming() const
	{
		constexpr uint32_t MaskValue = SysTick_CTRL_COUNTFLAG_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
		if (SysTick->CTRL & MaskValue == MaskValue)
			sysTickHook();
	}
	Tick SystemTimerClass::_GetTiming() const
	{
		return Tick((static_cast<uint64_t>(OverflowCount + 1) << 24) - SysTick->VAL << (SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk ? 0 : 3));
	}
	void SystemTimerClass::Delay(Tick Time)
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
				_Handler = &(HandlerA = [&OverflowCount]()
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
		_Handler = &(HandlerA = []()
					 { SysTick->CTRL = 0; });
		SysTick->LOAD = TimeTicks;
		SysTick->CTRL = CTRL;
		while (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
			;
	}
#endif
	using _SlowTick = std::chrono::duration<uint64_t, std::ratio<1, 29400>>; // 数据表说32768，实测明显不准。采用实测值。
#ifdef TOFA_REALTIMER
	RealTimerClass RealTimer;
	extern "C" void RTT_Handler()
	{
		if (RealTimer._Handler)
			(*RealTimer._Handler)();
		while (RTT->RTT_SR) // 读SR后不会马上清零，必须确认清零后才能退出，否则可能导致无限中断
			;
	}
	bool RealTimerClass::Busy() const
	{
		return static_cast<bool>(_Handler);
	}
	void RealTimerClass::Pause()
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
		if (!(RTT->RTT_MR & RTT_MR_ALMIEN) && _Handler)
			RTT->RTT_MR |= RTT_MR_ALMIEN | RTT_MR_RTTRST;
		// 可能存在问题：RTTRST可能导致AR变成-1
	}
	void RealTimerClass::Stop()
	{
		_Handler = nullptr;
		RTT->RTT_MR = 0;
	}
	void RealTimerClass::StartTiming()
	{
		RTT->RTT_AR = -1;
		RTT->RTT_MR = 1 | RTT_MR_ALMIEN | RTT_MR_RTTRST;
		OverflowCount = 0;
		_Handler = &(HandlerA = []()
					 {
						 if (++RealTimer.OverflowCount == 2)
						 {
							 const uint16_t RTPRES = RTT->RTT_MR << 1;
							 RTT->RTT_MR = RTPRES | RTT_MR_ALMIEN;
							 // 可能存在问题：MR的修改可能在RTTRST之前不会生效
							 RealTimer.OverflowCount = 1;
							 if (!RTPRES)
								 RealTimer._Handler = &RealTimer.HandlerB;
						 }
						 // 可能存在问题：中断寄存器尚未清空导致中断再次触发
					 });
		HandlerB = []()
		{ RealTimer.OverflowCount++; };
		while (RTT->RTT_SR)
			;
		NVIC_EnableIRQ(RTT_IRQn);
	}
	void RealTimerClass::RefreshTiming() const
	{
		if (RTT->RTT_SR & RTT_SR_ALMS && RTT->RTT_MR & RTT_MR_ALMIEN)
			RTT_Handler();
	}
	Tick RealTimerClass::_GetTiming() const
	{
		uint64_t TimerTicks = (static_cast<uint64_t>(OverflowCount + 1) << 32) + RTT->RTT_VR - RTT->RTT_AR;
		if (const uint16_t RTPRES = RTT->RTT_MR)
			TimerTicks *= RTPRES;
		else
			TimerTicks <<= 16;
		return std::chrono::duration_cast<Tick>(_SlowTick(TimerTicks));
	}
	void RealTimerClass::Delay(Tick Time)
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
		_Handler = &(HandlerA = [&OverflowCount]()
					 {
				if (!--OverflowCount) {
					RealTimer._Handler = nullptr;
					RTT->RTT_MR = 0;
				} });
		NVIC_EnableIRQ(RTT_IRQn);
		while (OverflowCount)
			;
	}
#endif
	void PeripheralTimerClass::Pause()
	{
		const uint8_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk;
		if (TCCLKS < TC_CMR_TCCLKS_XC0)
		{
			this->TCCLKS = TCCLKS;
			Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | TC_CMR_TCCLKS_XC0;
		}
	}
	void PeripheralTimerClass::Continue() const
	{
		if ((Channel.TC_CMR & TC_CMR_TCCLKS_Msk) > TC_CMR_TCCLKS_TIMER_CLOCK5)
			Channel.TC_CMR = Channel.TC_CMR & ~TC_CMR_TCCLKS_Msk | TCCLKS;
	}
#define _TOFA_HandlerDef(Index)                                                  \
	extern "C" void TC##Index##_Handler()                                        \
	{                                                                            \
		PeripheralTimers[(size_t)_PeripheralEnum::Timer##Index].RefreshTiming(); \
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
	void PeripheralTimerClass::Initialize()
	{
		if (Uninitialized)
		{
			PMC->PMC_WPMR = PMC_WPMR_WPKEY_VALUE;
			pmc_enable_periph_clk(UL_ID_TC);
			NVIC_EnableIRQ(irq);
			// TC_WPMR默认不写保护
			Uninitialized = false;
		}
	}
	void PeripheralTimerClass::StartTiming()
	{
		Channel.TC_IDR = -1;
		Initialize();
		Channel.TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
		Channel.TC_CMR = TC_CMR_WAVSEL_UP | TC_CMR_WAVE;
		OverflowCount = 0;
		_Handler = &(HandlerA = [this]()
					 {
				if (++OverflowCount == 4) {
					if ((++Channel.TC_CMR & TC_CMR_TCCLKS_Msk) == TC_CMR_TCCLKS_TIMER_CLOCK4)
						_Handler = &HandlerB;
					OverflowCount = 1;
				} });
		HandlerB = [this]()
		{
			if (++OverflowCount == (F_CPU >> 7) / 32768)
			{
				++Channel.TC_CMR;
				OverflowCount = 1;
				_Handler = &(HandlerA = [&OverflowCount = OverflowCount]()
							 { OverflowCount++; });
			}
		};
		Channel.TC_IER = TC_IER_COVFS;
	}
	void PeripheralTimerClass::RefreshTiming() const
	{
		if (Channel.TC_SR & Channel.TC_IMR)
			(*_Handler)();
	}
	Tick PeripheralTimerClass::_GetTiming() const
	{
		const uint64_t TimeTicks = ((uint64_t)OverflowCount << 32) + Channel.TC_CV;
		uint32_t TCCLKS = Channel.TC_CMR & TC_CMR_TCCLKS_Msk;
		if (TCCLKS > TC_CMR_TCCLKS_TIMER_CLOCK5)
			TCCLKS = TCCLKS;
		return TCCLKS == TC_CMR_TCCLKS_TIMER_CLOCK5 ? std::chrono::duration_cast<Tick>(_SlowTick(TimeTicks)) : Tick(TimeTicks << (TCCLKS << 1) + 1);
	}
	void PeripheralTimerClass::Delay(Tick Time)
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
				_Handler = &(HandlerA = [this, &OverflowCount]
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
	PeripheralTimerClass PeripheralTimers[] =
		{
#ifdef TOFA_TIMER0
			{TC0->TC_CHANNEL[0], TC0_IRQn, ID_TC0},
#endif
#ifdef TOFA_TIMER1
			{TC0->TC_CHANNEL[1], TC1_IRQn, ID_TC1},
#endif
#ifdef TOFA_TIMER2
			{TC0->TC_CHANNEL[2], TC2_IRQn, ID_TC2},
#endif
#ifdef TOFA_TIMER3
			{TC1->TC_CHANNEL[0], TC3_IRQn, ID_TC3},
#endif
#ifdef TOFA_TIMER4
			{TC1->TC_CHANNEL[1], TC4_IRQn, ID_TC4},
#endif
#ifdef TOFA_TIMER5
			{TC1->TC_CHANNEL[2], TC5_IRQn, ID_TC5},
#endif
#ifdef TOFA_TIMER6
			{TC2->TC_CHANNEL[0], TC6_IRQn, ID_TC6},
#endif
#ifdef TOFA_TIMER7
			{TC2->TC_CHANNEL[1], TC7_IRQn, ID_TC7},
#endif
#ifdef TOFA_TIMER8
			{TC2->TC_CHANNEL[2], TC8_IRQn, ID_TC8},
#endif
	};
#endif
#define _TOFA_Reference &&
#define _TOFA_Capture(Variable) Variable = std::move(Variable)
#define _TOFA_DirectHandler &(HandlerA = std::move(Do))
#define _TOFA_PublicCache(Callback) PublicCache = std::move(Callback)
#define _TOFA_ThisCapture(Callback)
#define _TOFA_ThisReference(Callback) PublicCache
#define _TOFA_SingletonCapture(Callback)
#define _TOFA_SingletonReference(Instance, Callback) Instance.PublicCache
#define _TOFA_SelfCapture(Callback) , &Callback = PublicCache
#define _TOFA_Mutable mutable
#include "_TOFA_RTO_Define.hpp"
#define _TOFA_Reference &
#define _TOFA_Capture(Variable) &Variable
#define _TOFA_DirectHandler &Do
#define _TOFA_PublicCache(Callback)
#define _TOFA_ThisCapture(Callback) , &Callback
#define _TOFA_ThisReference(Callback) Callback
#define _TOFA_SingletonCapture(Callback) , &Callback
#define _TOFA_SingletonReference(Instance, Callback) Callback
#define _TOFA_SelfCapture(Callback) , &Callback
#define _TOFA_Mutable
#include "_TOFA_RTO_Define.hpp"
}