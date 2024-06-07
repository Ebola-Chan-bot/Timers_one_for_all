#pragma once
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
#endif
namespace Timers_one_for_all
{
	enum class TimerEnum
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
#ifdef TOFA_SYSTIMER
		SysTimer,
#endif
		_NumTimers
	};
	constexpr uint8_t NumTimers = (uint8_t)TimerEnum::_NumTimers;
#ifdef ARDUINO_ARCH_AVR
	struct _TimerState
	{
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
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	constexpr uint64_t InfiniteRepeat = -1;
	struct TimerClass
	{
		_TimerState &State; // 全局可变类型的引用是不可变的，所以可以放在不可变对象中
		volatile uint8_t &TCCRB;
		volatile uint8_t &TIMSK;
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。
		bool Busy() const { return TIMSK; }
		// 暂停计时器。暂停一个已暂停的计时器将不做任何事
		void Pause() const
		{
			const uint8_t Clock = TCCRB & 0b111;
			if (Clock)
			{
				State.Clock = Clock;
				TCCRB &= ~0b111;
			}
		}
		// 继续计时器。继续一个未暂停的计时器将不做任何事
		void Continue() const
		{
			if (!(TCCRB & 0b111))
				TCCRB |= State.Clock;
		}
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。
		void Stop() const { TIMSK = 0; }
		// 开始计时任务。稍后可用GetTiming获取已记录的时间。
		virtual void StartTiming() const = 0;
		// 获取已记录的时间，模板参数指定要返回的std::chrono::duration时间格式。
		template <typename T>
		T GetTiming() const { return std::chrono::duration_cast<T>(GetTiming()); }
		// 阻塞Duration时长。在主线程中调用将仅阻塞主线程，可以被其它线程中断。在中断线程中调用将阻塞所有线程，无法被中断，可能导致其它依赖中断的任务出现未定义行为。注意，Arduino内置delay函数不能在中断中使用，但本函数确实可以在中断中使用。此方法一定会覆盖计时器的上一个任务，即使时长为0
		template <typename T>
		void Delay(T Duration) const { Delay(std::chrono::duration_cast<Tick>(Duration)); }
		// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务。如果延时为0，此方法直接执行Do，计时器的上一个任务不会被覆盖。
		template <typename T>
		void DoAfter(T After, std::function<void()> Do) const { DoAfter(std::chrono::duration_cast<Tick>(After), Do); }
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。如果重复次数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, uint64_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			RepeatEvery(std::chrono::duration_cast<Tick>(Every), Do, RepeatTimes, DoneCallback);
		}
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。在指定的持续时间结束后执行回调。如果持续时间为0，此方法立即执行DoneCallback（如果有），不会覆盖计时器的上一个任务。
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, T RepeatDuration, std::function<void()> DoneCallback = nullptr) const
		{
			const T TimeLeft = RepeatDuration % Every;
			RepeatEvery(Every, Do, RepeatDuration / Every, DoneCallback								? [this, TimeLeft, DoneCallback]()
															   { DoAfter(TimeLeft, DoneCallback); } : []() {});
		}
		// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定半周期数（即NumHalfPeriods为DoA和DoB被执行的次数之和，如果指定为奇数则DoA会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果重复半周期数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), DoA, std::chrono::duration_cast<Tick>(AfterB), DoB, NumHalfPeriods, DoneCallback)
		}
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, T RepeatDuration, std::function<void()> DoneCallback = nullptr) const
		{
			const T CycleLeft = RepeatDuration % (AfterA + AfterB);
			const T HalfLeft = CycleLeft % AfterA;
			DoubleRepeat(AfterA, DoA, AfterB, DoB, RepeatDuration / (AfterA + AfterB) * 2 + CycleLeft / AfterA, DoneCallback							? [this, HalfLeft, DoneCallback]
																													{ DoAfter(HalfLeft, DoneCallback) } : []() {});
		}
		template <typename T>
		void RandomRepeat(
			std::function<T()> RandomGenerator, std::function<void()> Do, uint64_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			RandomRepeat([RandomGenerator]()
						 { return std::chrono::duration_cast<Tick>(RandomGenerator()); },
						 Do, RepeatTimes, DoneCallback);
		}
		template <typename T>
		void RandomRepeat(
			std::function<T()> RandomGenerator, std::function<void()> Do, T RepeatDuration, std::function<void()> DoneCallback = []() {}) const
		{
			RandomRepeat([RandomGenerator]()
						 { return std::chrono::duration_cast<Tick>(RandomGenerator()); },
						 Do, std::chrono::duration_cast<Tick>(RepeatDuration), DoneCallback);
		}

	protected:
		constexpr TimerClass(_TimerState &State, volatile uint8_t &TCCRB, volatile uint8_t &TIMSK) : State(State), TCCRB(TCCRB), TIMSK(TIMSK) {}
		virtual Tick GetTiming() const = 0;
		virtual void Delay(Tick) const = 0;
		virtual void DoAfter(Tick After, std::function<void()> Do) const = 0;
		virtual void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const = 0;
		virtual void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const = 0;
		virtual void RandomRepeat(std::function<Tick()> RandomGenerator, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const = 0;
		virtual void RandomRepeat(std::function<Tick()> RandomGenerator, std::function<void()> Do, Tick RepeatDuration, std::function<void()> DoneCallback) const = 0;
	};
#ifdef TOFA_TIMER0
	struct TimerClass0 : public TimerClass
	{
		constexpr TimerClass0(_TimerState &State, volatile uint8_t &TCCRB, volatile uint8_t &TIMSK) : TimerClass(State, TCCRB, TIMSK) {}
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
	constexpr TimerClass0 HardwareTimer0(_TimerStates[(size_t)TimerEnum::Timer0], TCCR0B, TIMSK0);
#endif
	struct TimerClass1 : public TimerClass
	{
		volatile uint8_t &TCCRA;
		volatile uint8_t &TIFR;
		volatile uint16_t &TCNT;
		volatile uint16_t &OCRA;
		volatile uint16_t &OCRB;
		constexpr TimerClass1(_TimerState &State, volatile uint8_t &TCCRA, volatile uint8_t &TCCRB, volatile uint8_t &TIMSK, volatile uint8_t &TIFR, volatile uint16_t &TCNT, volatile uint16_t &OCRA, volatile uint16_t &OCRB) : TimerClass(State, TCCRB, TIMSK), TCCRA(TCCRA), TIFR(TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB) {}
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
#ifdef TOFA_TIMER1
	constexpr TimerClass1 HardwareTimer1(_TimerStates[(size_t)TimerEnum::Timer1], TCCR1A, TCCR1B, TIMSK1, TIFR1, TCNT1, OCR1A, OCR1B);
#endif
#ifdef TOFA_TIMER2
	struct TimerClass2 : public TimerClass
	{
		constexpr TimerClass2(_TimerState &State, volatile uint8_t &TCCRB, volatile uint8_t &TIMSK) : TimerClass(State, TCCRB, TIMSK) {}
		void StartTiming() const override;
		Tick GetTiming() const override;
		void Delay(Tick) const override;
		void DoAfter(Tick After, std::function<void()> Do) const override;
		void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const override;
		void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const override;
	};
	constexpr TimerClass2 HardwareTimer2(_TimerStates[(size_t)TimerEnum::Timer2], TCCR2B, TIMSK2);
#endif
#ifdef TOFA_TIMER3
	constexpr TimerClass1 HardwareTimer3(_TimerStates[(size_t)TimerEnum::Timer3], TCCR3A, TCCR3B, TIMSK3, TIFR3, TCNT3, OCR3A, OCR3B);
#endif
#ifdef TOFA_TIMER4
	constexpr TimerClass1 HardwareTimer4(_TimerStates[(size_t)TimerEnum::Timer4], TCCR4A, TCCR4B, TIMSK4, TIFR4, TCNT4, OCR4A, OCR4B);
#endif
#ifdef TOFA_TIMER5
	constexpr TimerClass1 HardwareTimer5(_TimerStates[(size_t)TimerEnum::Timer5], TCCR5A, TCCR5B, TIMSK5, TIFR5, TCNT5, OCR5A, OCR5B);
#endif
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
	// 如果没有空闲的计时器，返回nullptr
	const TimerClass *AllocateIdleTimer();
}