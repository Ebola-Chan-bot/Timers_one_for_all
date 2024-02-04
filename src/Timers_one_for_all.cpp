#include "Timers_one_for_all.hpp"
namespace Timers_one_for_all
{
	Exception AllocateTimer(uint8_t &Timer)
	{
		Timer = NumTimers - 1;
		do
		{
			if (!TimerBusy[Timer])
			{
				TimerBusy[Timer] = true;
				return Exception::Successful_operation;
			}
		} while (--Timer < NumTimers);
		return Exception::No_idle_timer;
	}
}