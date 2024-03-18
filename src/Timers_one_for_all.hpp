#pragma once
#include "TimingTask.hpp"
namespace Timers_one_for_all
{
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