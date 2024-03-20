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
		// 清零并启动/重启计时器
		virtual void Start() const = 0;
		// 停止计时器。已记录的时间不会丢失，可以调用Continue继续计时。
		virtual void Stop() const = 0;
		// 继续已停止的计时器。在Start之前调用此方法是未定义行为
		virtual void Continue() const = 0;
		// 析构时将会自动终止任务
		virtual ~DynamicTimingTask() {}
		// 使用指定的硬件计时器创建任务。不检查是否空闲，不会使计时器忙碌。指定不可用的硬件号是未定义行为。指定正在执行其他任务的计时器是未定义行为。使用此方法意味着用户决定手动调度计时器，可以获得比自动调度更高的性能，一般不应当与自动调度方法一起使用。
		static DynamicTimingTask *Create(uint8_t HardwareIndex);
		// 尝试占用一个计时器以创建任务。如果没有空闲的计时器，返回nullptr
		static DynamicTimingTask *TryCreate();
		// 尝试占用指定的软件计时器创建任务。如果计时器忙碌或无效，返回nullptr
		static DynamicTimingTask *TryCreate(uint8_t SoftwareIndex);

	protected:
		size_t OverflowCount;
		uint8_t PrescalerIndex;
		virtual Tick GetTicks() const = 0;
	};
	template <typename TimerType, typename T = std::make_integer_sequence<uint8_t, TimerType::NumPrescalers - 1>>
	constexpr uint8_t _SwitchThreshold[TimerType::NumPrescalers - 1];
	template <typename TimerType, uint8_t... Index>
	constexpr uint8_t _SwitchThreshold<TimerType, std::integer_sequence<uint8_t, Index...>>[TimerType::NumPrescalers - 1] = {1 << TimerType::Prescalers[Index + 1] - TimerType::Prescalers[Index]...};
	// 此类在编译器确定硬件计时器。额外指定构造/析构时是否占用/释放对应的软件号。
	template <uint8_t HardwareIndex, bool SetSoftwareTimer = false>
	struct StaticTimingTask : public DynamicTimingTask
	{
		void Start() const override
		{
			HardwareTimer<HardwareIndex>.TCNT = 0;
			HardwareTimer<HardwareIndex>.TCCRB = 1;
			HardwareTimer<HardwareIndex>.TCCRA = 0;
			PrescalerIndex = 0;
			HardwareTimer<HardwareIndex>.TIFR = -1;
			HardwareTimer<HardwareIndex>.TIMSK = 1 << TOIE1;
			OverflowCount = 0;
			_TimerInterrupts[HardwareIndex].Overflow = [this]()
			{ this->OverflowIsr(); };
		}
		void Stop() const override
		{
			HardwareTimer<HardwareIndex>.TCCRB = 0;
		}
		void Continue() const override
		{
			HardwareTimer<HardwareIndex>.TCCRB = PrescalerIndex + 1;
		}
		~StaticTimingTask()
		{
			HardwareTimer<HardwareIndex>.TIMSK = 0;
			if (SetSoftwareTimer)
				FreeHardwareTimer(HardwareIndex);
		}

	protected:
		Tick GetTicks() const override
		{
			return (OverflowCount << 16) + HardwareTimer<HardwareIndex>.TCNT << HardwareTimer1::Prescalers[PrescalerIndex];
		}
		void OverflowIsr()
		{
			if (++OverflowCount == _SwitchThreshold<HardwareTimer1>[PrescalerIndex])
			{
				HardwareTimer<HardwareIndex>.TCCRB = ++PrescalerIndex + 1;
				OverflowCount = 1;
				if (PrescalerIndex == HardwareTimer1::NumPrescalers - 1)
					_TimerInterrupts[HardwareIndex].Overflow = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
	template <bool SetSoftwareTimer>
	struct StaticTimingTask<0, SetSoftwareTimer> : public DynamicTimingTask
	{
		void Start() const override
		{
			HardwareTimer<0>.TCNT = 0;
			HardwareTimer<0>.TCCRB = 1;
			HardwareTimer<0>.TCCRA = 0;
			HardwareTimer<0>.OCRA = -1;
			PrescalerIndex = 0;
			HardwareTimer<0>.TIFR = -1;
			HardwareTimer<0>.TIMSK = 1 << OCIE0A;
			OverflowCount = 0;
			_TimerInterrupts[0].CompareA = [this]()
			{ this->OverflowIsr(); };
		}
		void Stop() const override
		{
			HardwareTimer<0>.TCCRB = 0;
		}
		void Continue() const override
		{
			HardwareTimer<0>.TCCRB = PrescalerIndex + 1;
		}
		~StaticTimingTask()
		{
			HardwareTimer<0>.TIMSK = 0;
			if (SetSoftwareTimer)
				FreeHardwareTimer(0);
		}

	protected:
		Tick GetTicks() const override
		{
			const uint8_t TCNT = HardwareTimer<0>.TCNT;
			return (TCNT == UINT8_MAX ? OverflowCount - 1 : OverflowCount << 8) + TCNT << HardwareTimer0::Prescalers[PrescalerIndex];
		}
		void OverflowIsr()
		{
			if (++OverflowCount == _SwitchThreshold<HardwareTimer0>[PrescalerIndex])
			{
				HardwareTimer<0>.TCCRB = ++PrescalerIndex + 1;
				OverflowCount = 1;
				if (PrescalerIndex == HardwareTimer0::NumPrescalers - 1)
					_TimerInterrupts[0].CompareA = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
#endif
}