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
		std::function<void()> OVFCOMPA;
		std::function<void()> COMPB;
		uint32_t OverflowCount;
		uint32_t OverflowTarget;
		uint64_t RepeatLeft;
	};
	extern _TimerState _TimerStates[NumTimers];
	using Tick = std::chrono::duration<uint64_t, std::ratio<1, F_CPU>>;
	constexpr uint64_t InfiniteRepeat = -1;
	struct TimerClass
	{
		_TimerState &State;
		uint8_t &TCCRA;
		uint8_t &TCCRB;
		uint8_t &TIMSK;
		uint8_t &TIFR;
		// 检查计时器是否忙碌。暂停的计时器也属于忙碌。
		bool Busy() const { return TIMSK; }
		// 暂停计时器。暂停一个已暂停的计时器将不做任何事
		void Pause() const;
		// 继续计时器。继续一个未暂停的计时器将不做任何事
		void Continue() const;
		// 终止计时器并设为空闲。一旦终止，任务将不能恢复。
		void Stop() const { TIMSK = 0; }
		// 开始计时任务。稍后可用GetTiming获取已记录的时间。
		virtual void StartTiming() const = 0;
		// 获取已记录的时间，模板参数指定要返回的std::chrono::duration时间格式。
		template <typename T>
		T GetTiming() const { return std::chrono::duration_cast<T>(GetTiming()); }
		virtual void Delay(Tick) const = 0;
		// 阻塞Duration时长。在主线程中调用将仅阻塞主线程，可以被其它线程中断。在中断线程中调用将阻塞所有线程，无法被中断，可能导致其它依赖中断的任务出现未定义行为。
		template <typename T>
		void Delay(T Duration) const { Delay(std::chrono::duration_cast<Tick>(Duration)); }
		// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务
		template <typename T>
		void DoAfter(T After, std::function<void()> Do) const { DoAfter(std::chrono::duration_cast<Tick>(After), Do); }
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, uint64_t RepeatTimes = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			RepeatEvery(std::chrono::duration_cast<Tick>(Every), Do, RepeatTimes, DoneCallback);
		}
		// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后，但仅持续
		template <typename T>
		void RepeatEvery(
			T Every, std::function<void()> Do, T RepeatDuration, std::function<void()> DoneCallback = []() {}) const
		{
			RepeatEvery(std::chrono::duration_cast<Tick>(Every), Do, RepeatDuration, DoneCallback);
		}
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::function<void()> DoneCallback = []() {}) const
		{
			DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), DoA, std::chrono::duration_cast<Tick>(AfterB), DoB, NumHalfPeriods, DoneCallback)
		}
		template <typename T>
		void DoubleRepeat(
			T AfterA, std::function<void()> DoA, T AfterB, std::function<void()> DoB, T RepeatDuration, std::function<void()> DoneCallback = []() {}) const
		{
			DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), DoA, std::chrono::duration_cast<Tick>(AfterB), DoB, RepeatDuration, DoneCallback)
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
		constexpr TimerClass(_TimerState &State, uint8_t &TCCRA, uint8_t &TCCRB, uint8_t &TIMSK, uint8_t &TIFR) : State(State), TCCRA(TCCRA), TCCRB(TCCRB), TIMSK(TIMSK), TIFR(TIFR) {}
		virtual Tick GetTiming() const = 0;
		virtual void DoAfter(Tick After, std::function<void()> Do) const = 0;
		virtual void RepeatEvery(Tick Every, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const = 0;
		virtual void RepeatEvery(Tick Every, std::function<void()> Do, Tick RepeatDuration, std::function<void()> DoneCallback) const = 0;
		virtual void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, uint64_t NumHalfPeriods, std::function<void()> DoneCallback) const = 0;
		virtual void DoubleRepeat(Tick AfterA, std::function<void()> DoA, Tick AfterB, std::function<void()> DoB, Tick RepeatDuration, std::function<void()> DoneCallback) const = 0;
		virtual void RandomRepeat(std::function<Tick()> RandomGenerator, std::function<void()> Do, uint64_t RepeatTimes, std::function<void()> DoneCallback) const = 0;
		virtual void RandomRepeat(std::function<Tick()> RandomGenerator, std::function<void()> Do, Tick RepeatDuration, std::function<void()> DoneCallback) const = 0;
	};
	template <typename T>
	struct _TimerBitClass : public TimerClass
	{
	protected:
		T &TCNT;
		T &OCRA;
		T &OCRB;
		constexpr _TimerBitClass(_TimerState &State, uint8_t &TCCRA, uint8_t &TCCRB, uint8_t &TIMSK, uint8_t &TIFR, T &TCNT, T &OCRA, T &OCRB) : TimerClass(State, TCCRA, TCCRB, TIMSK, TIFR), TCNT(TCNT), OCRA(OCRA), OCRB(OCRB) {}
	};
#ifdef TOFA_TIMER0
	struct TimerClass0 : public _TimerBitClass<uint8_t>
	{
		constexpr TimerClass0(_TimerState &State, uint8_t &TCCRA, uint8_t &TCCRB, uint8_t &TIMSK, uint8_t &TIFR, uint8_t &TCNT, uint8_t &OCRA, uint8_t &OCRB) : _TimerBitClass(State, TCCRA, TCCRB, TIMSK, TIFR, TCNT, OCRA, OCRB) {}
		void StartTiming() const override;
		Tick GetTiming() const override;
	};
	constexpr TimerClass0 HardwareTimer0(_TimerStates[(size_t)TimerEnum::Timer0], (uint8_t &)TCCR0A, (uint8_t &)TCCR0B, (uint8_t &)TIMSK0, (uint8_t &)TIFR0, (uint8_t &)TCNT0, (uint8_t &)OCR0A, (uint8_t &)OCR0B);
#endif
	struct TimerClass1 : public _TimerBitClass<uint16_t>
	{
		constexpr TimerClass1(_TimerState &State, uint8_t &TCCRA, uint8_t &TCCRB, uint8_t &TIMSK, uint8_t &TIFR, uint16_t &TCNT, uint16_t &OCRA, uint16_t &OCRB) : _TimerBitClass(State, TCCRA, TCCRB, TIMSK, TIFR, TCNT, OCRA, OCRB) {}
		void StartTiming() const override;
	};
#ifdef TOFA_TIMER1
	constexpr TimerClass1 HardwareTimer1(_TimerStates[(size_t)TimerEnum::Timer1], (uint8_t &)TCCR1A, (uint8_t &)TCCR1B, (uint8_t &)TIMSK1, (uint8_t &)TIFR1, (uint16_t &)TCNT1, (uint16_t &)OCR1A, (uint16_t &)OCR1B);
#endif
#ifdef TOFA_TIMER2
	struct TimerClass2 : public _TimerBitClass<uint8_t>
	{
		constexpr TimerClass2(_TimerState &State, uint8_t &TCCRA, uint8_t &TCCRB, uint8_t &TIMSK, uint8_t &TIFR, uint8_t &TCNT, uint8_t &OCRA, uint8_t &OCRB) : _TimerBitClass(State, TCCRA, TCCRB, TIMSK, TIFR, TCNT, OCRA, OCRB) {}
		void StartTiming() const override;
	};
	constexpr TimerClass2 HardwareTimer2(_TimerStates[(size_t)TimerEnum::Timer2], (uint8_t &)TCCR2A, (uint8_t &)TCCR2B, (uint8_t &)TIMSK2, (uint8_t &)TIFR2, (uint8_t &)TCNT2, (uint8_t &)OCR2A, (uint8_t &)OCR2B);
#endif
#ifdef TOFA_TIMER3
	constexpr TimerClass1 HardwareTimer3(_TimerStates[(size_t)TimerEnum::Timer3], (uint8_t &)TCCR3A, (uint8_t &)TCCR3B, (uint8_t &)TIMSK3, (uint8_t &)TIFR3, (uint16_t &)TCNT3, (uint16_t &)OCR3A, (uint16_t &)OCR3B);
#endif
#ifdef TOFA_TIMER4
	constexpr TimerClass1 HardwareTimer4(_TimerStates[(size_t)TimerEnum::Timer4], (uint8_t &)TCCR4A, (uint8_t &)TCCR4B, (uint8_t &)TIMSK4, (uint8_t &)TIFR4, (uint16_t &)TCNT4, (uint16_t &)OCR4A, (uint16_t &)OCR4B);
#endif
#ifdef TOFA_TIMER5
	constexpr TimerClass1 HardwareTimer5(_TimerStates[(size_t)TimerEnum::Timer5], (uint8_t &)TCCR5A, (uint8_t &)TCCR5B, (uint8_t &)TIMSK5, (uint8_t &)TIFR5, (uint16_t &)TCNT5, (uint16_t &)OCR5A, (uint16_t &)OCR5B);
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