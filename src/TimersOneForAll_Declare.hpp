#pragma once
#include <Cpp_Standard_Library.h>
#include <chrono>
#include <memory>
#include <Arduino.h>
namespace Timers_one_for_all
{
	// 枚举可用的计时器
	enum class TimerEnum
	{
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_SYSTIMER
		SysTimer,
#endif
#ifdef TOFA_REALTIMER
		RealTimer,
#endif
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
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_TIMER6
		Timer6,
#endif
#ifdef TOFA_TIMER7
		Timer7,
#endif
#ifdef TOFA_TIMER8
		Timer8,
#endif
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
		const size_t Value;
	};
#endif
	struct TimerClass
	{
		bool Allocatable = true;
#ifdef ARDUINO_ARCH_AVR
		// 暂停计时器，使其相关功能暂时失效，但可以用Continue恢复，不会将计时器设为空闲。暂停一个已暂停的计时器将不做任何事。
		void Pause()
		{
			const uint8_t Clock = TCCRB & 0b111;
			if (Clock)
			{
				this->Clock = Clock;
				TCCRB &= ~0b111;
			}
		}
		// 继续计时器。继续一个未暂停的计时器将不做任何事
		void Continue() const
		{
			if (!(TCCRB & 0b111))
				TCCRB |= Clock;
		}
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。此操作不会改变计时器是否接受自动分配的状态。如果需要用新任务覆盖计时器上正在执行的其它任务，可以直接布置那个任务而无需先Stop。Stop确保上次布置任务的中断处理代码不再被执行，但不会还原已被任务修改的全局状态（如全局变量、引脚电平等）。
		virtual void Stop() const = 0;
		const std::move_only_function<void() const> *_COMPA;
		const std::move_only_function<void() const> *_COMPB;
#endif
#ifdef ARDUINO_ARCH_SAM
		// 暂停计时器，使其相关功能暂时失效，但可以用Continue恢复，不会将计时器设为空闲。暂停一个已暂停的计时器将不做任何事。
		virtual void Pause() = 0;
		// 继续计时器。继续一个非处于暂停状态的计时器将不做任何事
		virtual void Continue() const = 0;
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。此操作不会改变计时器是否接受自动分配的状态。如果需要用新任务覆盖计时器上正在执行的其它任务，可以直接布置那个任务而无需先Stop。Stop确保上次布置任务的中断处理代码不再被执行，但不会还原已被任务修改的全局状态（如全局变量、引脚电平等）。
		virtual void Stop() = 0;
		const std::move_only_function<void() const> *_Handler = nullptr;
#endif
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。忙碌的计时器也可能被自动分配，应以Allocatable的返回值为准
		virtual bool Busy() const = 0;
		// 开始计时任务。稍后可用GetTiming获取已记录的时间。
		virtual void StartTiming() = 0;
		// 获取从上次调用StartTiming以来经历的时间，排除中间暂停的时间。模板参数指定要返回的std::chrono::duration时间格式。如果上次StartTiming之后还调用了Stop或布置了其它任务，此方法将产生未定义行为。
		template <typename T>
		T GetTiming() const { return std::chrono::duration_cast<T>(GetTiming()); }
		// 阻塞Duration时长。此方法一定会覆盖计时器的上一个任务，即使时长为0。此方法只能在主线程中使用，在中断处理函数中使用可能会永不返回。
		template <typename T>
		void Delay(T Duration) { Delay(std::chrono::duration_cast<Tick>(Duration)); }
		// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务。此方法一定会覆盖计时器的上一个任务，即使延时为0。
		template <typename T>
		void DoAfter(T After, std::move_only_function<void() const> &&Do) { DoAfter(std::chrono::duration_cast<Tick>(After), std::move(Do)); }
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。如果重复次数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void RepeatEvery(
			T Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes = InfiniteRepeat, std::move_only_function<void() const> &&DoneCallback = []() {})
		{
			RepeatEvery(std::chrono::duration_cast<Tick>(Every), std::move(Do), RepeatTimes, std::move(DoneCallback));
		}
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。在指定的持续时间结束后执行回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
		template <typename T>
		void RepeatEvery(
			T Every, std::move_only_function<void() const> &&Do, T RepeatDuration, std::move_only_function<void() const> &&DoneCallback = nullptr)
		{
			const T TimeLeft = RepeatDuration % Every;
			if (DoneCallback)
				RepeatEvery(Every, std::move(Do), RepeatDuration / Every, [this, TimeLeft, &DoneCallback]()
							{ DoAfter(TimeLeft, std::move(DoneCallback)); });
			else
				RepeatEvery(Every, std::move(Do), RepeatDuration / Every);
		}
		// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定半周期数（即NumHalfPeriods为DoA和DoB被执行的次数之和，如果指定为奇数则DoA会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果重复半周期数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::move_only_function<void() const> &&DoA, T AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::move_only_function<void() const> &&DoneCallback = []() {})
		{
			DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), std::move(DoA), std::chrono::duration_cast<Tick>(AfterB), std::move(DoB), NumHalfPeriods, std::move(DoneCallback));
		}
		// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定时长（时间到后立即停止，因此DoA可能会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::move_only_function<void() const> &&DoA, T AfterB, std::move_only_function<void() const> &&DoB, T RepeatDuration, std::move_only_function<void() const> &&DoneCallback = nullptr)
		{
			const T CycleLeft = RepeatDuration % (AfterA + AfterB);
			const T HalfLeft = CycleLeft % AfterA;
			DoubleRepeat(AfterA, std::move(DoA), AfterB, std::move(DoB), RepeatDuration / (AfterA + AfterB) * 2 + CycleLeft / AfterA, DoneCallback										  ? [this, HalfLeft, &DoneCallback]
																																		  { DoAfter(HalfLeft, std::move(DoneCallback)); } : []() {});
		}
		TimerClass &operator=(const TimerClass &) = delete;
		TimerClass &operator=(TimerClass &&) = delete;
		TimerClass(const TimerClass &) = delete;
		TimerClass(TimerClass &&) = delete;

	protected:
		virtual ~TimerClass() = default;
		std::move_only_function<void() const> HandlerA;
		std::move_only_function<void() const> HandlerB;
#ifdef ARDUINO_ARCH_AVR
		uint8_t Clock;
		uint32_t OverflowCountA;
		uint32_t OverflowCountB;
		_RuntimeReference<uint8_t> const TCCRB;
		TimerClass(_RuntimeReference<uint8_t> TCCRB) : TCCRB(TCCRB) {}
#endif
#ifdef ARDUINO_ARCH_SAM
		TimerClass() = default;
		uint32_t OverflowCount;
#endif
		uint64_t RepeatLeft;
		virtual Tick GetTiming() const = 0;
		virtual void Delay(Tick) = 0;
		virtual void DoAfter(Tick After, std::move_only_function<void() const> &&Do) = 0;
		virtual void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) = 0;
		virtual void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) = 0;
	};
#ifdef ARDUINO_ARCH_AVR
#define _MMIO_BYTE(mem_addr) mem_addr
#define _MMIO_WORD(mem_addr) mem_addr
#ifdef TOFA_TIMER0
	// 0号计时器
	extern struct TimerClass0 : TimerClass
	{
		TimerClass0(_RuntimeReference<uint8_t> TCCRB) : TimerClass(TCCRB) {}
		bool Busy() const override;
		void Stop() const override;
		void StartTiming() override;

	protected:
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
	} HardwareTimer0;
#endif
	struct TimerClass1 : TimerClass
	{
		TimerClass1(_RuntimeReference<uint8_t> TCCRA, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK, _RuntimeReference<uint8_t> TIFR, _RuntimeReference<uint16_t> TCNT, _RuntimeReference<uint16_t> OCRA, _RuntimeReference<uint16_t> OCRB) : TimerClass(TCCRB), TCCRA(TCCRA), TIFR(TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB), TIMSK(TIMSK) {}
		bool Busy() const override;
		void Stop() const override;
		void StartTiming() override;
		const std::move_only_function<void() const> *_OVF;

	protected:
		_RuntimeReference<uint8_t> const TCCRA;
		_RuntimeReference<uint8_t> const TIFR;
		_RuntimeReference<uint8_t> const TIMSK;
		_RuntimeReference<uint16_t> const TCNT;
		_RuntimeReference<uint16_t> const OCRA;
		_RuntimeReference<uint16_t> const OCRB;
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
	};
#ifdef TOFA_TIMER1
	// 1号计时器
	extern TimerClass1 HardwareTimer1;
#endif
#ifdef TOFA_TIMER2
	// 2号计时器
	extern struct TimerClass2 : public TimerClass
	{
		TimerClass2(_RuntimeReference<uint8_t> TCCRB) : TimerClass(TCCRB) {}
		bool Busy() const override;
		void Stop() const override;
		void StartTiming() override;
		const std::move_only_function<void() const> *_OVF;

	protected:
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
	} HardwareTimer2;
#endif
#ifdef TOFA_TIMER3
	// 3号计时器
	extern TimerClass1 HardwareTimer3;
#endif
#ifdef TOFA_TIMER4
	// 4号计时器
	extern TimerClass1 HardwareTimer4;
#endif
#ifdef TOFA_TIMER5
	// 5号计时器
	extern TimerClass1 HardwareTimer5;
#endif
#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(mem_addr))
#define _MMIO_WORD(mem_addr) (*(volatile uint16_t *)(mem_addr))
	// 所有硬件计时器。此数组可用TimerEnum转换成size_t后直接索引到对应的指针。
	constexpr TimerClass *HardwareTimers[] = {
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
	extern struct SystemTimerClass : TimerClass
	{
		bool Busy() const override;
		void Pause() override;
		void Continue() const override;
		void Stop() override;
		void StartTiming() override;

	protected:
		uint32_t VAL;
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
	} SystemTimer;
#endif
#ifdef TOFA_REALTIMER
	// 实时计时器
	extern struct RealTimerClass : TimerClass
	{
		bool Busy() const override;
		void Pause() override;
		void Continue() const override;
		void Stop() override;
		void StartTiming() override;

	protected:
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
	} RealTimer;
#endif
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
	// 周边计时器
	extern struct PeripheralTimerClass : TimerClass
	{
		bool Busy() const override { return Channel.TC_SR & TC_SR_CLKSTA; }
		void Pause() override;
		void Continue() const override;
		void Stop() override { Channel.TC_CCR = TC_CCR_CLKDIS; }
		void StartTiming() override;
		PeripheralTimerClass(TcChannel &Channel, IRQn_Type irq, uint32_t UL_ID_TC) : Channel(Channel), irq(irq), UL_ID_TC(UL_ID_TC) {}
		void _ClearAndHandle() const;
		TcChannel &Channel;

	protected:
		bool Uninitialized = true;
		uint32_t TCCLKS;
		IRQn_Type irq;
		uint32_t UL_ID_TC;
		Tick GetTiming() const override;
		void Delay(Tick) override;
		void DoAfter(Tick After, std::move_only_function<void() const> &&Do) override;
		void RepeatEvery(Tick Every, std::move_only_function<void() const> &&Do, uint64_t RepeatTimes, std::move_only_function<void() const> &&DoneCallback) override;
		void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> &&DoA, Tick AfterB, std::move_only_function<void() const> &&DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> &&DoneCallback) override;
		void Initialize();
	} PeripheralTimers[static_cast<size_t>(_PeripheralEnum::NumPeripherals)];
	// 所有可用的硬件计时器。此数组可用TimerEnum转换为size_t索引以获取对应指针。所有计时器默认接受自动分配。
	constexpr TimerClass *HardwareTimers[] =
		{
#ifdef TOFA_SYSTIMER
			&SystemTimer,
#endif
#ifdef TOFA_REALTIMER
			&RealTimer,
#endif
#ifdef TOFA_TIMER0
			&PeripheralTimers[0],
#define _TOFA_Index1 1
#else
#define _TOFA_Index1 0
#endif
#ifdef TOFA_TIMER1
			&PeripheralTimers[_TOFA_Index1],
#define _TOFA_Index2 _TOFA_Index1 + 1
#else
#define _TOFA_Index2 _TOFA_Index1
#endif
#ifdef TOFA_TIMER2
			&PeripheralTimers[_TOFA_Index2],
#define _TOFA_Index3 _TOFA_Index2 + 1
#else
#define _TOFA_Index3 _TOFA_Index2
#endif
#ifdef TOFA_TIMER3
			&PeripheralTimers[_TOFA_Index3],
#define _TOFA_Index4 _TOFA_Index3 + 1
#else
#define _TOFA_Index4 _TOFA_Index3
#endif
#ifdef TOFA_TIMER4
			&PeripheralTimers[_TOFA_Index4],
#define _TOFA_Index5 _TOFA_Index4 + 1
#else
#define _TOFA_Index5 _TOFA_Index4
#endif
#ifdef TOFA_TIMER5
			&PeripheralTimers[_TOFA_Index5],
#define _TOFA_Index6 _TOFA_Index5 + 1
#else
#define _TOFA_Index6 _TOFA_Index5
#endif
#ifdef TOFA_TIMER6
			&PeripheralTimers[_TOFA_Index6],
#define _TOFA_Index7 _TOFA_Index6 + 1
#else
#define _TOFA_Index7 _TOFA_Index6
#endif
#ifdef TOFA_TIMER7
			&PeripheralTimers[_TOFA_Index7],
#define _TOFA_Index8 _TOFA_Index7 + 1
#else
#define _TOFA_Index8 _TOFA_Index7
#endif
#ifdef TOFA_TIMER8
			&PeripheralTimers[_TOFA_Index8],
#endif
	};
#endif
	// 分配一个接受自动分配的计时器。如果没有这样的计时器，返回nullptr。此方法优先分配序数较大的的计时器（对SAM架构，优后分配实时计时器和系统计时器）。在进入setup之前调用此方法是未定义行为。被此方法分配返回的计时器将不再接受自动分配，需要手动设置Allocatable才能使其重新接受自动分配。
	TimerClass *AllocateTimer();
	// 分配一个接受自动分配的计时器unique_ptr。如果没有这样的计时器，返回指针值为nullptr。此方法优先分配序数较大的的计时器（对SAM架构，优后分配实时计时器和系统计时器）。在进入setup之前调用此方法是未定义行为。此unique_ptr析构前计时器将不再接受自动分配。
	inline std::unique_ptr<TimerClass, void (*)(TimerClass *)> AllocateTimerUnique()
	{
		return std::unique_ptr<TimerClass, void (*)(TimerClass *)>{AllocateTimer(), [](TimerClass *T)
																   {
																	   if (T)
																		   T->Allocatable = true;
																   }};
	}
}