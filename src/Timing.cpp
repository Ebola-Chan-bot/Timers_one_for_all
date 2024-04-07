#include "TimingTask.hpp"
#ifdef ARDUINO_ARCH_AVR
using namespace Timers_one_for_all;
template <typename T = std::make_integer_sequence<uint8_t, NumTimers>>
struct Creators;
template <uint8_t... SoftwareIndex>
struct Creators<std::integer_sequence<uint8_t, SoftwareIndex...>>
{
	static constexpr DynamicTimingTask *(*Static[NumTimers])() = {[]() -> DynamicTimingTask *
																  { return new StaticTimingTask<SoftwareToHardware[SoftwareIndex], false>; }...};
	static constexpr DynamicTimingTask *(*Dynamic[NumTimers])() = {[]() -> DynamicTimingTask *
																   { return new StaticTimingTask<SoftwareToHardware[SoftwareIndex], true>; }...};
};
DynamicTimingTask *DynamicTimingTask::Create(uint8_t SoftwareIndex)
{
	return Creators<>::Static[SoftwareIndex]();
}
DynamicTimingTask *DynamicTimingTask::TryCreate()
{
	const uint8_t SoftwareIndex = AllocateSoftwareTimer();
	if (SoftwareIndex < NumTimers)
		return Creators<>::Dynamic[SoftwareIndex]();
	else
		return nullptr;
}
DynamicTimingTask *DynamicTimingTask::TryCreate(uint8_t SoftwareIndex)
{
	if (TimerStates[SoftwareIndex].TimerFree)
		return Creators<>::Dynamic[SoftwareIndex]();
	else
		return nullptr;
}
#endif