#pragma once
#include "Kernel.hpp"
namespace Timers_one_for_all
{
#ifdef ARDUINO_ARCH_AVR
	// 此类在运行时动态分配硬件计时器
	struct DynamicTimingTask
	{
		// 在Start之前调用此方法是未定义行为
		template <typename _Rep, typename _Period>
		void GetTiming(std::chrono::duration<_Rep, _Period> &Time) const
		{
			return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(GetTicks());
		}
		// 清零并启动计时器
		virtual void Start() const = 0;
		// 停止计时器。已记录的时间不会丢失，可以调用Continue继续计时。
		virtual void Stop() const = 0;
		// 在Start之前调用此方法是未定义行为
		virtual void Continue() const = 0;
		// 析构时将会自动终止任务
		virtual ~DynamicTimingTask() {}
		// 使用指定的硬件计时器创建任务。不检查是否空闲，不会使计时器忙碌。如果指定不可用的硬件号，将返回nullptr
		static DynamicTimingTask *Create(uint8_t HardwareIndex);
		// 尝试占用一个计时器以创建任务。如果没有空闲的计时器，返回nullptr
		static DynamicTimingTask *TryCreate();
		// 尝试占用指定的软件计时器创建任务。如果计时器忙碌或无效，返回nullptr
		static DynamicTimingTask *TryCreate(uint8_t SoftwareIndex);

	protected:
		size_t OverflowCount;
		virtual Tick GetTicks() const = 0;
	};
	// 此类在编译器确定硬件计时器。额外指定构造/析构时是否占用/释放对应的软件号。
	template <uint8_t HardwareIndex, typename std::enable_if<IsValidHardware<HardwareIndex>::value, bool>::type SetSoftwareTimer = false>
	struct StaticTimingTask : public DynamicTimingTask
	{
		void Start() const override
		{
		}
		void Stop() const override
		{
		}
		void Continue() const override
		{
		}
		~StaticTimingTask() {}

	protected:
		Tick GetTicks() const override
		{
		}
	};
#endif
}