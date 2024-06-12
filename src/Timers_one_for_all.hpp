#pragma once
#include <Cpp_Standard_Library.h>
#include <chrono>
#include <functional>
#include <Arduino.h>
#define TOFA_TIMER0
#define TOFA_TIMER1
#define TOFA_TIMER2
#define TOFA_TIMER3
#define TOFA_TIMER4
#define TOFA_TIMER5
#ifdef ARDUINO_ARCH_SAM
#define TOFA_TIMER6
#define TOFA_TIMER7
#define TOFA_TIMER8
#define TOFA_SYSTIMER
#define TOFA_REALTIMER
#endif
namespace Timers_one_for_all
{
	// 枚举可用的计时器
	enum class TimerEnum
	{
#ifdef TOFA_SYSTIMER
		SysTimer,
#endif
#ifdef TOFA_REALTIMER
		RealTimer,
#endif
#ifdef TOFA_TIMER0
		Timer0,
#endif
#ifdef TOFA_TIMER1
		Timer1,
#endif
#ifdef TOFA_TIMER2
		Timer2,
#endif
#ifdef TOFA_TIMER3
		Timer3,
#endif
#ifdef TOFA_TIMER4
		Timer4,
#endif
#ifdef TOFA_TIMER5
		Timer5,
#endif
#ifdef TOFA_TIMER6
		Timer6,
#endif
#ifdef TOFA_TIMER7
		Timer7,
#endif
#ifdef TOFA_TIMER8
		Timer8,
#endif
		_NumTimers
	};
	// 可用的计时器个数
	constexpr uint8_t NumTimers = (uint8_t)TimerEnum::_NumTimers;
	// 计时器的最小精度单位
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	// 使用此常数表示无限重复，直到手动停止
	constexpr uint64_t InfiniteRepeat = -1;
#ifdef ARDUINO_ARCH_AVR
	struct _TimerState
	{
		bool Allocatable = true;
		uint8_t Clock;
		std::function<void()> OVF;
		std::function<void()> COMPA;
		std::function<void()> COMPB;
		std::function<void()> CandidateOVF;
		std::function<void()> CandidateCOMP;
		uint32_t OverflowCount;
		uint64_t RepeatLeft;
	};
	extern _TimerState _TimerStates[NumTimers];
	// 整数不能在编译期转换为constexpr引用，所以只能在运行时转换
	template <typename T>
	struct _RuntimeReference
	{
		operator volatile T &() const
		{
			return *(volatile T *)Value;
		}
		volatile T &operator=(T New) const
		{
			return *(volatile T *)Value = New;
		}
		constexpr _RuntimeReference(size_t Value) : Value(Value) {}

	protected:
		const size_t Value;
	};
#endif
	struct TimerClass
	{
#ifdef ARDUINO_ARCH_AVR
		_TimerState &_State; // 全局可变类型的引用是不可变的，所以可以放在不可变对象中
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。
		bool Busy() const { return TIMSK; }
		// 暂停计时器。暂停一个已暂停的计时器将不做任何事
		void Pause() const
		{
			const uint8_t Clock = TCCRB & 0b111;
			if (Clock)
			{
				_State.Clock = Clock;
				TCCRB &= ~0b111;
			}
		}
		// 继续计时器。继续一个未暂停的计时器将不做任何事
		void Continue() const
		{
			if (!(TCCRB & 0b111))
				TCCRB |= _State.Clock;
		}
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。
		void Stop() const { TIMSK = 0; }
		// 指示当前计时器是否接受自动分配
		bool Allocatable() const { return _State.Allocatable; }
		// 设置当前计时器是否接受自动分配
		void Allocatable(bool A) const { _State.Allocatable = A; }
#endif
#ifdef ARDUINO_ARCH_SAM
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。
		virtual bool Busy() const = 0;
		// 暂停计时器。暂停一个已暂停的计时器将不做任何事
		virtual void Pause() const = 0;
		// 继续计时器。继续一个未暂停的计时器将不做任何事
		virtual void Continue() const = 0;
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。
		virtual void Stop() const = 0;
		// 指示当前计时器是否接受自动分配
		virtual bool Allocatable() const = 0;
		// 设置当前计时器是否接受自动分配
		virtual void Allocatable(bool A) const = 0;
#endif
		// 开始计时任务。稍后可用GetTiming获取已记录的时间。
		virtual void StartTiming() const = 0;
		// 获取已记录的时间，模板参数指定要返回的std::chrono::duration时间格式。
		template <typename T>
		T GetTiming() const { return std::chrono::duration_cast<T>(GetTiming()); }
		// 阻塞Duration时长。在主线程中调用将仅阻塞主线程，可以被其它线程中断。在中断线程中调用将阻塞所有线程，无法被中断，可能导致其它依赖中断的任务出现未定义行为。注意，Arduino内置delay函数不能在中断中使用，但本函数确实可以在中断中使用。此方法一定会覆盖计时器的上一个任务，即使时长为0
		template <typename T>
		void Delay(T Duration) const { Delay(std::chrono::duration_cast<Tick>(Duration)); }
		// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务。此方法一定会覆盖计时器的上一个任务，即使延时为0
		template <typename T>
		void DoAfter(T After, std::function<void()> Do) const { DoAfter(std::chrono::duration_cast<Tick>(After), Do); }
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。如果重复次数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, uint64_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			RepeatEvery(std::chrono::duration_cast<Tick>(Every), Do, RepeatTimes, DoneCallback);
		}
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。在指定的持续时间结束后执行回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, T RepeatDuration, std::function<void()> DoneCallback = nullptr) const
		{
			const T TimeLeft = RepeatDuration % Every;
			if (DoneCallback)
				RepeatEvery(Every, Do, RepeatDuration / Every, [this, TimeLeft, DoneCallback]()
							{ DoAfter(TimeLeft, DoneCallback); });
			else
				RepeatEvery(Every, Do, RepeatDuration / Every);
		}
		// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定半周期数（即NumHalfPeriods为DoA和DoB被执行的次数之和，如果指定为奇数则DoA会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果重复半周期数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), DoA, std::chrono::duration_cast<Tick>(AfterB), DoB, NumHalfPeriods, DoneCallback);
		}
		// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定时长（时间到后立即停止，因此DoA可能会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, T RepeatDuration, std::function<void()> DoneCallback = nullptr) const
		{
			const T CycleLeft = RepeatDuration % (AfterA + AfterB);
			const T HalfLeft = CycleLeft % AfterA;
			DoubleRepeat(AfterA, DoA, AfterB, DoB, RepeatDuration / (AfterA + AfterB) * 2 + CycleLeft / AfterA, DoneCallback							 ? [this, HalfLeft, DoneCallback]
																													{ DoAfter(HalfLeft, DoneCallback); } : []() {});
		}

	protected:
#ifdef ARDUINO_ARCH_AVR
		_RuntimeReference<uint8_t> TCCRB;
		_RuntimeReference<uint8_t> TIMSK;
		constexpr TimerClass(_TimerState &_State, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK) : _State(_State), TCCRB(TCCRB), TIMSK(TIMSK) {}
#endif
		virtual Tick GetTiming() const = 0;
		virtual void Delay(Tick) const = 0;
		virtual void DoAfter(Tick After, std::function<void()> Do) const = 0;
		virtual void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const = 0;
		virtual void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const = 0;
	};
#ifdef ARDUINO_ARCH_AVR
#define _MMIO_BYTE(mem_addr) mem_addr
#define _MMIO_WORD(mem_addr) mem_addr
#ifdef TOFA_TIMER0
	struct TimerClass0 : public TimerClass
	{
		constexpr TimerClass0(_TimerState &_State, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK) : TimerClass(_State, TCCRB, TIMSK) {}

	protected:
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
	// 0号计时器
	constexpr TimerClass0 HardwareTimer0(_TimerStates[(size_t)TimerEnum::Timer0], TCCR0B, TIMSK0);
#endif
	struct TimerClass1 : public TimerClass
	{
		constexpr TimerClass1(_TimerState &_State, _RuntimeReference<uint8_t> TCCRA, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK, _RuntimeReference<uint8_t> TIFR, _RuntimeReference<uint16_t> TCNT, _RuntimeReference<uint16_t> OCRA, _RuntimeReference<uint16_t> OCRB) : TimerClass(_State, TCCRB, TIMSK), TCCRA(TCCRA), TIFR(TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB) {}

	protected:
		_RuntimeReference<uint8_t> TCCRA;
		_RuntimeReference<uint8_t> TIFR;
		_RuntimeReference<uint16_t> TCNT;
		_RuntimeReference<uint16_t> OCRA;
		_RuntimeReference<uint16_t> OCRB;
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
#ifdef TOFA_TIMER1
	// 1号计时器
	constexpr TimerClass1 HardwareTimer1(_TimerStates[(size_t)TimerEnum::Timer1], TCCR1A, TCCR1B, TIMSK1, TIFR1, TCNT1, OCR1A, OCR1B);
#endif
#ifdef TOFA_TIMER2
	struct TimerClass2 : public TimerClass
	{
		constexpr TimerClass2(_TimerState &_State, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK) : TimerClass(_State, TCCRB, TIMSK) {}

	protected:
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
	// 2号计时器
	constexpr TimerClass2 HardwareTimer2(_TimerStates[(size_t)TimerEnum::Timer2], TCCR2B, TIMSK2);
#endif
#ifdef TOFA_TIMER3
	// 3号计时器
	constexpr TimerClass1 HardwareTimer3(_TimerStates[(size_t)TimerEnum::Timer3], TCCR3A, TCCR3B, TIMSK3, TIFR3, TCNT3, OCR3A, OCR3B);
#endif
#ifdef TOFA_TIMER4
	// 4号计时器
	constexpr TimerClass1 HardwareTimer4(_TimerStates[(size_t)TimerEnum::Timer4], TCCR4A, TCCR4B, TIMSK4, TIFR4, TCNT4, OCR4A, OCR4B);
#endif
#ifdef TOFA_TIMER5
	// 5号计时器
	constexpr TimerClass1 HardwareTimer5(_TimerStates[(size_t)TimerEnum::Timer5], TCCR5A, TCCR5B, TIMSK5, TIFR5, TCNT5, OCR5A, OCR5B);
#endif
#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(mem_addr))
#define _MMIO_WORD(mem_addr) (*(volatile uint16_t *)(mem_addr))
	// 所有硬件计时器。此数组可用TimerEnum转换成size_t后直接索引到对应的指针。
	constexpr const TimerClass *HardwareTimers[] = {
#ifdef TOFA_TIMER0
		&HardwareTimer0,
#endif
#ifdef TOFA_TIMER1
		&HardwareTimer1,
#endif
#ifdef TOFA_TIMER2
		&HardwareTimer2,
#endif
#ifdef TOFA_TIMER3
		&HardwareTimer3,
#endif
#ifdef TOFA_TIMER4
		&HardwareTimer4,
#endif
#ifdef TOFA_TIMER5
		&HardwareTimer5,
#endif
	};
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
	// 系统计时器
	constexpr struct SystemTimerClass : public TimerClass
	{
		bool Busy() const override;
		void Pause() const override;
		void Continue() const override;
		void Stop() const override;
		void StartTiming() const override;
		bool Allocatable() const override;
		void Allocatable(bool A) const override;

	protected:
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	} SystemTimer;
#endif
#ifdef TOFA_REALTIMER
	// 实时计时器
	constexpr struct RealTimerClass : public TimerClass
	{
		bool Busy() const override;
		void Pause() const override;
		void Continue() const override;
		void Stop() const override;
		void StartTiming() const override;
		bool Allocatable() const override;
		void Allocatable(bool A) const override;

	protected:
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	} RealTimer;
#endif
	struct _TimerState
	{
		bool Allocatable = true;
		std::function<void()> Handler;
		uint64_t RepeatLeft;
	};
	struct _PeripheralState : public _TimerState
	{
		bool Uninitialized = true;
		uint32_t TCCLKS;
		uint32_t OverflowCount;
		std::function<void()> CandidateHandler;
	};
	enum class _PeripheralEnum
	{
#ifdef TOFA_TIMER0
		Timer0,
#endif
#ifdef TOFA_TIMER1
		Timer1,
#endif
#ifdef TOFA_TIMER2
		Timer2,
#endif
#ifdef TOFA_TIMER3
		Timer3,
#endif
#ifdef TOFA_TIMER4
		Timer4,
#endif
#ifdef TOFA_TIMER5
		Timer5,
#endif
#ifdef TOFA_TIMER6
		Timer6,
#endif
#ifdef TOFA_TIMER7
		Timer7,
#endif
#ifdef TOFA_TIMER8
		Timer8,
#endif
		NumPeripherals
	};
	extern _PeripheralState _TimerStates[(uint8_t)_PeripheralEnum::NumPeripherals];
	// 周边计时器
	constexpr struct PeripheralTimerClass : public TimerClass
	{
		_PeripheralState &_State;
		bool Busy() const override;
		void Pause() const override;
		void Continue() const override;
		void Stop() const override;
		void StartTiming() const override;
		bool Allocatable() const override;
		void Allocatable(bool A) const override;
		constexpr PeripheralTimerClass(_PeripheralState &_State, TcChannel &Channel, IRQn_Type irq, uint32_t UL_ID_TC) : _State(_State), Channel(Channel), irq(irq), UL_ID_TC(UL_ID_TC) {}

	protected:
		TcChannel &Channel;
		IRQn_Type irq;
		uint32_t UL_ID_TC;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
		void Initialize() const;
	} PeripheralTimers[] =
		{
#ifdef TOFA_TIMER0
			{_TimerStates[(size_t)_PeripheralEnum::Timer0], TC0->TC_CHANNEL[0], TC0_IRQn, ID_TC0},
#endif
#ifdef TOFA_TIMER1
			{_TimerStates[(size_t)_PeripheralEnum::Timer1], TC0->TC_CHANNEL[1], TC1_IRQn, ID_TC1},
#endif
#ifdef TOFA_TIMER2
			{_TimerStates[(size_t)_PeripheralEnum::Timer2], TC0->TC_CHANNEL[2], TC2_IRQn, ID_TC2},
#endif
#ifdef TOFA_TIMER3
			{_TimerStates[(size_t)_PeripheralEnum::Timer3], TC1->TC_CHANNEL[0], TC3_IRQn, ID_TC3},
#endif
#ifdef TOFA_TIMER4
			{_TimerStates[(size_t)_PeripheralEnum::Timer4], TC1->TC_CHANNEL[1], TC4_IRQn, ID_TC4},
#endif
#ifdef TOFA_TIMER5
			{_TimerStates[(size_t)_PeripheralEnum::Timer5], TC1->TC_CHANNEL[2], TC5_IRQn, ID_TC5},
#endif
#ifdef TOFA_TIMER6
			{_TimerStates[(size_t)_PeripheralEnum::Timer6], TC2->TC_CHANNEL[0], TC6_IRQn, ID_TC6},
#endif
#ifdef TOFA_TIMER7
			{_TimerStates[(size_t)_PeripheralEnum::Timer7], TC2->TC_CHANNEL[1], TC7_IRQn, ID_TC7},
#endif
#ifdef TOFA_TIMER8
			{_TimerStates[(size_t)_PeripheralEnum::Timer8], TC2->TC_CHANNEL[2], TC8_IRQn, ID_TC8},
#endif
	};
	// 所有可用的硬件计时器。此数组可用TimerEnum转换为size_t索引以获取对应指针。
	constexpr const TimerClass *HardwareTimers[] =
		{
#ifdef TOFA_SYSTIMER
			&SystemTimer,
#endif
#ifdef TOFA_REALTIMER
			&RealTimer,
#endif
#ifdef TOFA_TIMER0
			&PeripheralTimers[0],
#endif
#ifdef TOFA_TIMER1
			&PeripheralTimers[1],
#endif
#ifdef TOFA_TIMER2
			&PeripheralTimers[2],
#endif
#ifdef TOFA_TIMER3
			&PeripheralTimers[3],
#endif
#ifdef TOFA_TIMER4
			&PeripheralTimers[4],
#endif
#ifdef TOFA_TIMER5
			&PeripheralTimers[5],
#endif
#ifdef TOFA_TIMER6
			&PeripheralTimers[6],
#endif
#ifdef TOFA_TIMER7
			&PeripheralTimers[7],
#endif
#ifdef TOFA_TIMER8
			&PeripheralTimers[8],
#endif
	};
#endif
	// 分配空闲的计时器。如果没有空闲的计时器，返回nullptr。此方法优先返回HardwareTimers中序数较大的的计时器。在进入setup之前的全局变量初始化阶段，不能调用此方法。
	const TimerClass *AllocateTimer();
}