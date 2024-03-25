#pragma once
#include "Kernel.hpp"
#include <limits>
#include <Arduino.h>
namespace Timers_one_for_all
{
	// 此类在运行时动态分配计时器。使用Create或TryCreate获取对象指针。使用delete删除成功构造的对象，删除时会自动终止任务。此类在AVR架构上是抽象类，不允许直接构造对象。在SAM架构上可以无参构造，但不同于Create或TryCreate，直接构造得到的对象将使用系统计时器而非外周计时器执行任务。
	struct DynamicTimingTask
	{
		// 在Start之前调用此方法是未定义行为
		template <typename _Rep, typename _Period>
		void GetTiming(std::chrono::duration<_Rep, _Period> &Time) const
		{
			return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(GetTicks());
		}
		// 清零并启动/重启计时器
		virtual void Start() const;
		// 停止计时器。已记录的时间不会丢失，可以调用Continue继续计时。
		virtual void Stop() const;
		// 继续已停止的计时器。在Start之前调用此方法是未定义行为
		virtual void Continue() const;
		// 析构时将会自动终止任务。
		virtual ~DynamicTimingTask() {}
		// 使用指定的软件计时器创建任务。使用此方法意味着用户决定手动调度计时器，一般不应当与自动调度方法一起使用。不检查是否空闲，不会使计时器忙碌。指定不可用的编号是未定义行为。指定正在执行其他任务的计时器是未定义行为。使用此方法创建的对象，析构时也不会使计时器空闲。
		static DynamicTimingTask *Create(uint8_t SoftwareIndex);
		// 尝试占用一个计时器以创建任务。如果没有空闲的计时器，返回nullptr。使用此方法创建的对象，析构时会使计时器空闲。
		static DynamicTimingTask *TryCreate();
		// 尝试占用指定的计时器创建任务。如果计时器忙碌或无效，返回nullptr。使用此方法创建的对象，析构时会使计时器空闲。
		static DynamicTimingTask *TryCreate(uint8_t SoftwareIndex);
#ifdef ARDUINO_ARCH_SAM
		DynamicTimingTask() {}
#endif

	protected:
		size_t OverflowCount;
		uint8_t PrescalerClock;
		virtual Tick GetTicks() const;
	};
	template <typename TimerType, typename T = std::make_integer_sequence<uint8_t, TimerType::NumPrescalers - 1>>
	constexpr uint8_t _SwitchThreshold[TimerType::NumPrescalers - 1];
	template <typename TimerType, uint8_t... Index>
	constexpr uint8_t _SwitchThreshold<TimerType, std::integer_sequence<uint8_t, Index...>>[TimerType::NumPrescalers - 1] = {1 << TimerType::Prescalers[Index + 1] - TimerType::Prescalers[Index]...};
	// 此类在编译期确定要使用的硬件计时器。可以直接无参构造，模板参数需要指定硬件计时器号。构造不会导致计时器忙碌，用户如有需求只能手动设置TimerStates，且需要注意硬件号和软件号的对应关系。可选将模板参数FreeTimer设为true，以在析构时使对应TimerState空闲。
	template <uint8_t HardwareIndex, bool FreeTimer = false>
	struct StaticTimingTask : public DynamicTimingTask
	{
		// 清零并启动/重启计时器
		void Start() const override
		{
			_HardwareTimer<HardwareIndex>.TCNT = 0;
			_HardwareTimer<HardwareIndex>.TCCRB = 1;
			_HardwareTimer<HardwareIndex>.TCCRA = 0;
			PrescalerClock = 0;
			_HardwareTimer<HardwareIndex>.TIFR = -1;
			_HardwareTimer<HardwareIndex>.TIMSK = 1 << TOIE1;
			OverflowCount = 0;
			_TimerInterrupts[HardwareIndex].Overflow = [this]()
			{ this->OverflowIsr(); };
		}
		// 停止计时器。已记录的时间不会丢失，可以调用Continue继续计时。
		void Stop() const override
		{
			_HardwareTimer<HardwareIndex>.TCCRB = 0;
		}
		// 继续已停止的计时器。在Start之前调用此方法是未定义行为
		void Continue() const override
		{
			_HardwareTimer<HardwareIndex>.TCCRB = PrescalerClock + 1;
		}
		// 析构时将会自动终止任务。如果FreeTimer设为true，还会使计时器空闲。
		~StaticTimingTask()
		{
			_HardwareTimer<HardwareIndex>.TIMSK = 0;
			if (FreeTimer)
				FreeSoftwareTimer(HardwareToSoftware[HardwareIndex]);
		}

	protected:
		// 这样就不用偏特化2号计时器了
		using TimerType = decltype(_HardwareTimer<HardwareIndex>);
		Tick GetTicks() const override
		{
			return (OverflowCount << std::numeric_limits<std::remove_cvref_t<decltype(_HardwareTimer<HardwareIndex>.TCNT)>>::digits) + _HardwareTimer<HardwareIndex>.TCNT << TimerType::Prescalers[PrescalerClock];
		}
		void OverflowIsr()
		{
			if (++OverflowCount == _SwitchThreshold<TimerType>[PrescalerClock])
			{
				_HardwareTimer<HardwareIndex>.TCCRB = ++PrescalerClock + 1;
				OverflowCount = 1;
				if (PrescalerClock == TimerType::NumPrescalers - 1)
					_TimerInterrupts[HardwareToSoftware[HardwareIndex]].Overflow = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
	// 0号计时器的Overflow中断被占用，必需偏特化
	template <bool FreeTimer>
	struct StaticTimingTask<0, FreeTimer> : public DynamicTimingTask
	{
		void Start() const override
		{
			_HardwareTimer<0>.TCNT = 0;
			_HardwareTimer<0>.TCCRB = 1;
			_HardwareTimer<0>.TCCRA = 0;
			_HardwareTimer<0>.OCRA = -1;
			PrescalerClock = 0;
			_HardwareTimer<0>.TIFR = -1;
			_HardwareTimer<0>.TIMSK = 1 << OCIE0A;
			OverflowCount = 0;
			_TimerInterrupts[0].CompareA = [this]()
			{ this->OverflowIsr(); };
		}
		void Stop() const override
		{
			_HardwareTimer<0>.TCCRB = 0;
		}
		// 在Start之前调用此方法是未定义行为
		void Continue() const override
		{
			_HardwareTimer<0>.TCCRB = PrescalerClock + 1;
		}
		~StaticTimingTask()
		{
			_HardwareTimer<0>.TIMSK = 0;
			if (FreeTimer)
				FreeSoftwareTimer(HardwareToSoftware[0]);
		}

	protected:
		Tick GetTicks() const override
		{
			const uint8_t TCNT = _HardwareTimer<0>.TCNT;
			return (TCNT == UINT8_MAX ? OverflowCount - 1 : OverflowCount << 8) + TCNT << HardwareTimer0::Prescalers[PrescalerClock];
		}
		void OverflowIsr()
		{
			if (++OverflowCount == _SwitchThreshold<HardwareTimer0>[PrescalerClock])
			{
				_HardwareTimer<0>.TCCRB = ++PrescalerClock + 1;
				OverflowCount = 1;
				if (PrescalerClock == HardwareTimer0::NumPrescalers - 1)
					_TimerInterrupts[HardwareToSoftware[0]].CompareA = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
}