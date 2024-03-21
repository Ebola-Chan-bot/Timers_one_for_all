#include "Kernel.hpp"
using namespace Timers_one_for_all;
#ifdef ARDUINO_ARCH_AVR

#ifdef TOFA_USE_TIMER0
ISR(TIMER0_COMPA_vect) { TimerStates[_HardwareToSoftware<>::value[0]].CompareA(); }
ISR(TIMER0_COMPB_vect) { TimerStates[_HardwareToSoftware<>::value[0]].CompareB(); }
#endif
#define TOFA_USE_TIMER(N)                                                                   \
	ISR(TIMER##N##_OVF_vect) { TimerStates[_HardwareToSoftware<>::value[N]].Overflow(); }   \
	ISR(TIMER##N##_COMPA_vect) { TimerStates[_HardwareToSoftware<>::value[N]].CompareA(); } \
	ISR(TIMER##N##_COMPB_vect) { TimerStates[_HardwareToSoftware<>::value[N]].CompareB(); }
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
#ifdef ARDUINO_ARCH_SAM
#define TOFA_USE_TIMER(N) \
	void TC##N##_Handler() { TimerState<>::TimerStates[_HardwareToSoftware<>::value[N]].InterruptHandler(); }
#ifdef TOFA_USE_TIMER0
TOFA_USE_TIMER(0);
#endif
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
#ifdef TOFA_USE_TIMER6
TOFA_USE_TIMER(6);
#endif
#ifdef TOFA_USE_TIMER7
TOFA_USE_TIMER(7);
#endif
#ifdef TOFA_USE_TIMER8
TOFA_USE_TIMER(8);
#endif
#endif
namespace Timers_one_for_all
{
#ifdef ARDUINO_ARCH_AVR
	uint8_t AllocateSoftwareTimer()
	{
		for (uint8_t T = 0; T < NumTimers; ++T)
			if (TimerStates[T].TimerFree)
			{
				TimerStates[T].TimerFree = false;
				return T;
			}
		return NumTimers;
	}
	TimerState TimerStates[NumTimers];
#endif
#ifdef ARDUINO_ARCH_SAM
	uint8_t AllocateSoftwareTimer()
	{
		for (uint8_t T = 0; T < NumTimers; ++T)
			if (TimerState<>::TimerStates[T].TimerFree)
			{
				TimerState<>::TimerStates[T].TimerFree = false;
				return T;
			}
		return NumTimers;
	}
	TimerState<>::TimerState(uint8_t SoftwareIndex)
	{
		static bool HasInitialized = []()
		{
			pmc_set_writeprotect(false);
			pmc_enable_all_periph_clk();
			return true;
		}();
		NVIC_EnableIRQ(PeripheralTimers[SoftwareIndex].irq);
	}
#endif
}