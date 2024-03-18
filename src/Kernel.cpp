#include "Kernel.hpp"
namespace Timers_one_for_all
{
	TimerInterrupt TimerInterrupts[NumTimers];
	bool TimerFree[NumTimers] = {
#ifdef TOFA_USE_TIMER0
		true,
#endif
#ifdef TOFA_USE_TIMER1
		true,
#endif
#ifdef TOFA_USE_TIMER2
		true,
#endif
#ifdef TOFA_USE_TIMER3
		true,
#endif
#ifdef TOFA_USE_TIMER4
		true,
#endif
#ifdef TOFA_USE_TIMER5
		true,
#endif
#ifdef TOFA_USE_TIMER6
		true,
#endif
#ifdef TOFA_USE_TIMER7
		true,
#endif
#ifdef TOFA_USE_TIMER8
		true,
#endif
	};
#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_USE_TIMER0
	ISR(TIMER0_COMPA_vect) { Timers_one_for_all::TimerInterrupts[0].CompareA(); }
	ISR(TIMER0_COMPB_vect) { Timers_one_for_all::TimerInterrupts[0].CompareB(); }
#endif
#define TOFA_USE_TIMER(N)                                                             \
	ISR(TIMER##N##_OVF_vect) { Timers_one_for_all::TimerInterrupts[N].Overflow(); }   \
	ISR(TIMER##N##_COMPA_vect) { Timers_one_for_all::TimerInterrupts[N].CompareA(); } \
	ISR(TIMER##N##_COMPB_vect) { Timers_one_for_all::TimerInterrupts[N].CompareB(); }
#ifdef TOFA_USE_TIMER1
	TOFA_USE_TIMER(1);
#endif
#ifdef TOFA_USE_TIMER2
	TOFA_USE_TIMER(2);
#endif
#ifdef TOFA_USE_TIMER3
	TOFA_USE_TIMER(3);
#endif
#ifdef TOFA_USE_TIMER4
	TOFA_USE_TIMER(4);
#endif
#ifdef TOFA_USE_TIMER5
	TOFA_USE_TIMER(5);
#endif
#endif
	uint8_t AllocateSoftwareTimer()
	{
		for (uint8_t T = 0; T < NumTimers; ++T)
			if (TimerFree[T])
			{
				TimerFree[T] = false;
				return T;
			}
		return NumTimers;
	}
}