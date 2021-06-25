#pragma once
#ifndef ARDUINO_ARCH_AVR
#error 仅支持AVR架构
#endif
/*
本命名空间中所有函数均以计时器编号（0~5）为第一个模板参数。
一个计时器不能同时执行两个任务，后项任务开始之前会强制终止前项任务。但你可以将前项任务本身设定为对后项任务的启动，实现任务之间的无缝衔接，且不会造成堆栈溢出。
所有参数必须在编译时已知，不支持运行时动态选择计时器、调整时间或任务参数等。
*/
namespace TimersOneForAll
{
	//延迟指定毫秒数后执行动作
	template <uint8_t TimerCode, uint16_t AfterMillisecondes, void (*DoTask)()>
	void DoAfter();
	//每隔固定时间重复动作。第一次动作也将在该时间后执行。无限重复，直到调用ShutDown，或布置了新的计时任务。
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)()>
	void RepeatAfter();
	//每隔固定时间重复动作。第一次动作也将在该时间后执行。可指定重复次数。如果重复0次，等价于ShutDown；重复1次，等价于DoAfter。
	template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), uint8_t RepeatTimes>
	void RepeatAfter();
	//以当前为0时刻开始计时，随后访问MillisecondsElapsed变量取得经过的毫秒数
	template <uint8_t TimerCode>
	void StartTiming();
	//上次调用StartTiming后到现在所经过的毫秒数
	template <uint8_t TimerCode>
	static uint16_t MillisecondsElapsed;
	//在指定引脚上生成指定频率的方波，无限持续，直到调用ShutDown，或布置了新的计时任务。
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
	void PlayTone();
	//在指定引脚上生成指定频率的方波，持续指定的毫秒数
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds>
	void PlayTone();
	/*
	生成高低电平时长不等的方波，无限持续，直到调用ShutDown，或布置了新的计时任务。
	一个高低周期的时长若在17~4194㎳之间，不建议使用0或2计时器。序列总是以高电平开始，低电平结束。
	*/
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
	void SquareWave();
	/*
	生成高低电平时长不等的方波。可以指定重复周期数。如果重复0次，等价于ShutDown。
	一个高低周期的时长若在17~4194㎳之间，不建议使用0或2计时器。序列总是以高电平开始，低电平结束。
	*/
	template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, uint8_t RepeatTimes>
	void SquareWave();
	//终止所有计时任务
	template <uint8_t TimerCode>
	void ShutDown();
}