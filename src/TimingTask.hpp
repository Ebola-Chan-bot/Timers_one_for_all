#pragma once
#include "Kernel.hpp"
#include <limits>
namespace Timers_one_for_all
{
#ifdef ARDUINO_ARCH_AVR
	// 此类在运行时动态分配计时器。使用Create或TryCreate获取对象指针，不允许直接构造。使用delete删除成功构造的对象，删除时会自动终止任务。
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
		// 析构时将会自动终止任务。
		virtual ~DynamicTimingTask() {}
		// 使用指定的计时器创建任务。不检查是否空闲，不会使计时器忙碌。指定不可用的编号是未定义行为。指定正在执行其他任务的计时器是未定义行为。使用此方法意味着用户决定手动调度计时器，可以获得比自动调度更高的性能，一般不应当与自动调度方法一起使用。使用此方法创建的对象，析构时也不会使计时器空闲。
		static DynamicTimingTask *Create(uint8_t SoftwareIndex);
		// 尝试占用一个计时器以创建任务。如果没有空闲的计时器，返回nullptr。使用此方法创建的对象，析构时会使计时器空闲。
		static DynamicTimingTask *TryCreate();
		// 尝试占用指定的计时器创建任务。如果计时器忙碌或无效，返回nullptr。使用此方法创建的对象，析构时会使计时器空闲。
		static DynamicTimingTask *TryCreate(uint8_t SoftwareIndex);

	protected:
		size_t OverflowCount;
		uint8_t PrescalerClock;
		virtual Tick GetTicks() const = 0;
	};
	template <typename TimerType, typename T = std::make_integer_sequence<uint8_t, TimerType::NumPrescalers - 1>>
	constexpr uint8_t _SwitchThreshold[TimerType::NumPrescalers - 1];
	template <typename TimerType, uint8_t... Index>
	constexpr uint8_t _SwitchThreshold<TimerType, std::integer_sequence<uint8_t, Index...>>[TimerType::NumPrescalers - 1] = {1 << TimerType::Prescalers[Index + 1] - TimerType::Prescalers[Index]...};
	// 此类可以直接无参构造，模板参数需要指定计时器号。构造不会导致计时器忙碌，用户如有需求只能手动设置TimerBusy。可选将模板参数TimerFree设为true，以在析构时使计时器空闲。
	template <uint8_t SoftwareIndex, bool FreeTimer = false>
	struct StaticTimingTask : public DynamicTimingTask
	{
		// 清零并启动/重启计时器
		void Start() const override
		{
			SoftwareTimer<SoftwareIndex>.TCNT = 0;
			SoftwareTimer<SoftwareIndex>.TCCRB = 1;
			SoftwareTimer<SoftwareIndex>.TCCRA = 0;
			PrescalerClock = 0;
			SoftwareTimer<SoftwareIndex>.TIFR = -1;
			SoftwareTimer<SoftwareIndex>.TIMSK = 1 << TOIE1;
			OverflowCount = 0;
			_TimerInterrupts[SoftwareIndex].Overflow = [this]()
			{ this->OverflowIsr(); };
		}
		// 停止计时器。已记录的时间不会丢失，可以调用Continue继续计时。
		void Stop() const override
		{
			SoftwareTimer<SoftwareIndex>.TCCRB = 0;
		}
		// 继续已停止的计时器。在Start之前调用此方法是未定义行为
		void Continue() const override
		{
			SoftwareTimer<SoftwareIndex>.TCCRB = PrescalerClock + 1;
		}
		// 析构时将会自动终止任务。如果FreeTimer设为true，还会使计时器空闲。
		~StaticTimingTask()
		{
			SoftwareTimer<SoftwareIndex>.TIMSK = 0;
			if (FreeTimer)
				FreeSoftwareTimer(SoftwareIndex);
		}

	protected:
		// 这样就不用偏特化2号计时器了
		using TimerType = decltype(SoftwareTimer<SoftwareIndex>);
		Tick GetTicks() const override
		{
			return (OverflowCount << std::numeric_limits<std::remove_cvref_t<decltype(SoftwareTimer<SoftwareIndex>.TCNT)>>::digits) + SoftwareTimer<SoftwareIndex>.TCNT << TimerType::Prescalers[PrescalerClock];
		}
		void OverflowIsr()
		{
			if (++OverflowCount == _SwitchThreshold<TimerType>[PrescalerClock])
			{
				SoftwareTimer<SoftwareIndex>.TCCRB = ++PrescalerClock + 1;
				OverflowCount = 1;
				if (PrescalerClock == TimerType::NumPrescalers - 1)
					_TimerInterrupts[SoftwareIndex].Overflow = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
	// 0号计时器的Overflow中断被占用，必需偏特化
	template <bool FreeTimer>
	struct StaticTimingTask<_HardwareToSoftware<>::value[0], FreeTimer> : public DynamicTimingTask
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
		void Continue() const override
		{
			_HardwareTimer<0>.TCCRB = PrescalerClock + 1;
		}
		~StaticTimingTask()
		{
			_HardwareTimer<0>.TIMSK = 0;
			if (FreeTimer)
				FreeSoftwareTimer(_HardwareToSoftware<>::value[0]);
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
					_TimerInterrupts[_HardwareToSoftware<>::value[0]].CompareA = [this]()
					{ this->OverflowCount++; };
			}
		}
	};
#endif

}