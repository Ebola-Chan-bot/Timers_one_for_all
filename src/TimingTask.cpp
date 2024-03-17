#include "TimingTask.hpp"
#ifdef ARDUINO_ARCH_AVR
#include <map>
using namespace Timers_one_for_all;
template <uint8_t HardwareIndex, bool SetSoftwareIndex>
DynamicTimingTask *StaticCreate()
{
	return new StaticTimingTask<HardwareIndex, false>();
}
template <bool SetSoftwareIndex, typename T>
std::map<uint8_t, DynamicTimingTask *(*)()> StaticCreators;
template <bool SetSoftwareIndex, uint8_t... SoftwareIndex>
std::map<uint8_t, DynamicTimingTask *(*)()> StaticCreators<SetSoftwareIndex, std::integer_sequence<uint8_t, SoftwareIndex...>>{{HardwareTimers[SoftwareIndex], StaticCreate<HardwareTimers[SoftwareIndex], SetSoftwareIndex>}...};
DynamicTimingTask *DynamicTimingTask::Create(uint8_t HardwareIndex)
{
	return StaticCreators<false, std::make_integer_sequence<uint8_t, NumTimers>>[HardwareIndex]();
}
#endif