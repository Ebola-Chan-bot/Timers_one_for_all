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
		// 指示计时器是否接受自动分配。如果true，将可能被AllocateTimer方法选中。如果不希望该计时器被AllocateTimer选中，应将此值设为false。默认为true。
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

		//当前正在工作的处理函数指针。为nullptr表示没有正在工作的处理函数。
		std::move_only_function<void() const> const *_Handler = nullptr;
#endif
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。忙碌的计时器也可能被自动分配，应以Allocatable的返回值为准
		virtual bool Busy() const = 0;

		// 开始计时任务。稍后可用GetTiming获取已记录的时间。
		virtual void StartTiming() = 0;

		// 刷新计时器。在有中断场景下无需使用此方法。在无中断场景下，则必须经常调用此方法手动检查中断。
		virtual void RefreshTiming() const = 0;

		// 获取从上次调用StartTiming以来经历的时间，排除中间暂停的时间。模板参数指定要返回的std::chrono::duration时间格式。如果上次StartTiming之后还调用了Stop或布置了其它任务，此方法将产生未定义行为。
		template <typename T = Tick>
		T GetTiming() const { return std::chrono::duration_cast<T>(_GetTiming()); }

		// 阻塞Duration时长。此方法一定会覆盖计时器的上一个任务，即使时长为0。此方法只能在主线程中使用，在中断处理函数中使用可能会永不返回。
		template <typename T>
		void Delay(T Duration) { Delay(std::chrono::duration_cast<Tick>(Duration)); }

		TimerClass &operator=(const TimerClass &) = delete;
		TimerClass &operator=(TimerClass &&) = delete;
		TimerClass(const TimerClass &) = delete;
		TimerClass(TimerClass &&) = delete;

	protected:
		virtual ~TimerClass() = default;

		//供派生类实现用的通用缓存位置

		std::move_only_function<void() const> HandlerA;
		std::move_only_function<void() const> HandlerB;
		std::move_only_function<void() const> PublicCache;
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
		virtual Tick _GetTiming() const = 0;
		virtual void Delay(Tick) = 0;
#define _TOFA_Reference const &
#define _TOFA_StdMove(Object) Object
#include "_TOFA_RTO_Declare.hpp"
#define _TOFA_Reference &&
#define _TOFA_StdMove(Object) std::move(Object)
#include "_TOFA_RTO_Declare.hpp"
	};
#define _TOFA_OverrideDeclare(Reference)                                                                                                                                           \
	void DoAfter(Tick After, std::move_only_function<void() const> Reference Do) override;                                                                                         \
	void RepeatEvery(Tick Every, std::move_only_function<void() const> Reference Do, uint64_t RepeatTimes, std::move_only_function<void() const> Reference DoneCallback) override; \
	void DoubleRepeat(Tick AfterA, std::move_only_function<void() const> Reference DoA, Tick AfterB, std::move_only_function<void() const> Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void() const> Reference DoneCallback) override;
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
		void RefreshTiming() const override;

	protected:
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
	} HardwareTimer0;
#endif
	struct TimerClass1 : TimerClass
	{
		TimerClass1(_RuntimeReference<uint8_t> TCCRA, _RuntimeReference<uint8_t> TCCRB, _RuntimeReference<uint8_t> TIMSK, _RuntimeReference<uint8_t> TIFR, _RuntimeReference<uint16_t> TCNT, _RuntimeReference<uint16_t> OCRA, _RuntimeReference<uint16_t> OCRB) : TimerClass(TCCRB), TCCRA(TCCRA), TIFR(TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB), TIMSK(TIMSK) {}
		bool Busy() const override;
		void Stop() const override;
		void StartTiming() override;
		void RefreshTiming() const override;
		const std::move_only_function<void() const> *_OVF;

	protected:
		_RuntimeReference<uint8_t> const TCCRA;
		_RuntimeReference<uint8_t> const TIFR;
		_RuntimeReference<uint8_t> const TIMSK;
		_RuntimeReference<uint16_t> const TCNT;
		_RuntimeReference<uint16_t> const OCRA;
		_RuntimeReference<uint16_t> const OCRB;
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
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
		void RefreshTiming() const override;
		const std::move_only_function<void() const> *_OVF;

	protected:
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
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
		void RefreshTiming() const override;

	protected:
		uint32_t VAL;
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
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
		void RefreshTiming() const override;

	protected:
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
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
		TcChannel &Channel;
		void RefreshTiming() const override;

	protected:
		bool Uninitialized = true;
		uint32_t TCCLKS;
		IRQn_Type irq;
		uint32_t UL_ID_TC;
		Tick _GetTiming() const override;
		void Delay(Tick) override;
		_TOFA_OverrideDeclare(&&);
		_TOFA_OverrideDeclare(const &);
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