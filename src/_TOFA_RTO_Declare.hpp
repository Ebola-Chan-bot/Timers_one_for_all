public:
// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务。此方法一定会覆盖计时器的上一个任务，即使延时为0。
template <typename T>
void DoAfter(T After, std::move_only_function<void()> _TOFA_Reference Do) { DoAfter(std::chrono::duration_cast<Tick>(After), _TOFA_StdMove(Do)); }
// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。如果重复次数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
template <typename T>
void RepeatEvery(
	T Every, std::move_only_function<void()> _TOFA_Reference Do, uint64_t RepeatTimes = InfiniteRepeat, std::move_only_function<void()> _TOFA_Reference DoneCallback = _TOFA_DoNothing)
{
	RepeatEvery(std::chrono::duration_cast<Tick>(Every), _TOFA_StdMove(Do), RepeatTimes, _TOFA_StdMove(DoneCallback));
}
// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后，重复直到达到指定的总时长。
template <typename T>
void RepeatEvery(
	T Every, std::move_only_function<void()> _TOFA_Reference Do, T RepeatDuration)
{
	RepeatEvery(Every, _TOFA_StdMove(Do), RepeatDuration / Every);
}
// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定半周期数（即NumHalfPeriods为DoA和DoB被执行的次数之和，如果指定为奇数则DoA会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果重复半周期数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
template <typename T>
void DoubleRepeat(
	T AfterA, std::move_only_function<void()> _TOFA_Reference DoA, T AfterB, std::move_only_function<void()> _TOFA_Reference DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::move_only_function<void()> _TOFA_Reference DoneCallback = _TOFA_DoNothing)
{
	DoubleRepeat(std::chrono::duration_cast<Tick>(AfterA), _TOFA_StdMove(DoA), std::chrono::duration_cast<Tick>(AfterB), _TOFA_StdMove(DoB), NumHalfPeriods, _TOFA_StdMove(DoneCallback));
}
// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定时长（时间到后立即停止，因此DoA可能会比DoB多执行一次）。
template <typename T>
void DoubleRepeat(
	T AfterA, std::move_only_function<void()> _TOFA_Reference DoA, T AfterB, std::move_only_function<void()> _TOFA_Reference DoB, T RepeatDuration)
{
	T const Cycle = AfterA + AfterB;
	DoubleRepeat(AfterA, _TOFA_StdMove(DoA), AfterB, _TOFA_StdMove(DoB), RepeatDuration / Cycle * 2 + RepeatDuration % Cycle / AfterA);
}

protected:
virtual void DoAfter(Tick After, std::move_only_function<void()> _TOFA_Reference Do) = 0;
virtual void RepeatEvery(Tick Every, std::move_only_function<void()> _TOFA_Reference Do, uint64_t RepeatTimes, std::move_only_function<void()> _TOFA_Reference DoneCallback) = 0;
virtual void DoubleRepeat(Tick AfterA, std::move_only_function<void()> _TOFA_Reference DoA, Tick AfterB, std::move_only_function<void()> _TOFA_Reference DoB, uint64_t NumHalfPeriods, std::move_only_function<void()> _TOFA_Reference DoneCallback) = 0;