#pragma once
#include "TimingTask.hpp"
namespace Timers_one_for_all
{
	// 返回值是计时器的软件号，范围[0,NumTimers)。使用SchedulableTimers以将软件号映射为硬件号。如果没有空闲的计时器，返回NumTimers。
	const SoftwareTimer& AllocateSoftwareTimer();
	// 将指定软件号的计时器标记为空闲，使其可以被再分配，但不会停止任何执行中的进程。输入已空闲的软件号不做任何事。输入大于等于NumTimers的无效软件号是未定义行为。
	inline void FreeSoftwareTimer(uint8_t SoftwareIndex) { SchedulableTimers[SoftwareIndex].Free = true; }
}
#ifdef ARDUINO_ARCH_AVR
#define TOFA_USE_TIMER0                                                           \
	ISR(TIMER0_COMPA_vect) { Timers_one_for_all::TimerInterrupts[0].CompareA(); } \
	ISR(TIMER0_COMPB_vect) { Timers_one_for_all::TimerInterrupts[0].CompareB(); }
#define TOFA_USE_TIMER(N)                                                             \
	ISR(TIMER##N##_OVF_vect) { Timers_one_for_all::TimerInterrupts[N].Overflow(); }   \
	ISR(TIMER##N##_COMPA_vect) { Timers_one_for_all::TimerInterrupts[N].CompareA(); } \
	ISR(TIMER##N##_COMPB_vect) { Timers_one_for_all::TimerInterrupts[N].CompareB(); }
#define TOFA_USE_TIMER1 TOFA_USE_TIMER(1)
#define TOFA_USE_TIMER2 TOFA_USE_TIMER(2)
#define TOFA_USE_TIMER3 TOFA_USE_TIMER(3)
#define TOFA_USE_TIMER4 TOFA_USE_TIMER(4)
#define TOFA_USE_TIMER5 TOFA_USE_TIMER(5)
#endif
#define TOFA_SCHEDULABLE_TIMERS(...) std::dynarray<uint8_t> Timers_one_for_all::SchedulableTimers{__VA_ARGS__};