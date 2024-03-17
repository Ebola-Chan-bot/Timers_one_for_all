#include "Kernel.hpp"
namespace Timers_one_for_all
{
#ifdef ARDUINO_ARCH_AVR
	TimerInterrupt TimerInterrupts[NumHardwareTimers];
#endif
	uint8_t AllocateSoftwareTimer()
	{
		for (uint8_t T = 0; T < SchedulableTimers.size(); ++T)
			if (SchedulableTimers[T].Free)
			{
				SchedulableTimers[T].Free = false;
				return T;
			}
		return SchedulableTimers.size();
	}
}