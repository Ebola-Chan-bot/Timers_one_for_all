充分利用开发板上所有的计时器。

音响、方波、延迟任务、定时重复，这些任务都需要应用开发板上的计时器才能完成。有时你甚至需要多个计时器同步运行，实现多线程任务。但是，当前Arduino社区并没有提供比较完善的计时器运行库。它们能够执行的任务模式非常有限，而且用户无法指定具体要使用哪个计时器。其结果就是，经常有一些使用计时器的库发生冲突，或者和用户自己的应用发生冲突。本项目旨在将计时器可能需要使用的所有功能在所有计时器上实现，最关键的是允许用户手动指定要使用的硬件计时器，避免冲突。

# 硬件计时器
为了避免任何冲突，本库默认不会占用任何计时器，因而也无法使用。用户需要在包含头文件前通过宏定义指定要使用的硬件计时器。例如：
```C++
//AVR和SAM架构都支持的计时器
//#define TOFA_TIMER0
#define TOFA_TIMER1
//#define TOFA_TIMER2
#define TOFA_TIMER3
#define TOFA_TIMER4
#define TOFA_TIMER5

//SAM架构特有的计时器
#ifdef ARDUINO_ARCH_SAM
#define TOFA_TIMER6
#define TOFA_TIMER7
#define TOFA_TIMER8
//#define TOFA_REALTIMER
//#define TOFA_SYSTIMER
#endif

#include <Timers_one_for_all.hpp>
using namespace Timers_one_for_all;
```
用户应当查询Arduino标准库文档和任何其它第三方库文档，确认那些库占用了哪些计时器。不要定义那些被占用的计时器宏，这样本库就不会使用那些计时器，也就不会和那些库发生冲突。
## ARDUINO_ARCH_AVR
此架构编译器必须启用C++17。打开“%LOCALAPPDATA%\Arduino15\packages\arduino\hardware\avr\<版本号>\platform.txt”并将参数“-std=gnu++11”更改为-std=gnu++17。此架构最多支持0~5共6个计时器。
### TOFA_TIMER0
`HardwareTimer0`，该计时器有8位，即$2^8=256$个计时状态，支持COMPA和COMPB中断，但不支持OVF中断，因为该中断被内置函数`millis();delay();micros();`占用了。本库考虑到这个情况，用COMPA和COMPB中断同样能实现所有的计时功能，因此该计时器仍然可用，但会付出一些微妙的性能代价。此外，一旦使用此计时器后，再使用相关内置函数将产生未定义行为。因此，您仍应避免使用该计时器，除非其它计时器都处于繁忙状态。
### TOFA_TIMER1 TOFA_TIMER3 TOFA_TIMER4 TOFA_TIMER5
`HardwareTimer1/3/4/5`，这些计时器都具有16位，即65536个计时状态，因此比8位计时器更精确。COMPA、COMPB和OVF三个中断都为可用。绝大多数情况下，这些计时器是您的首选。注意，3~5号计时器仅在Mega2560系列开发板中支持。
### TOFA_TIMER2
`HardwareTimer2`该计时器也是8位，但和0号计时器有些方面不同：
- 该计时器支持7种预分频模式，而所有其它计时器都只支持5种。因此该计时器比0号略微精确一些，但仍不如16位计时器。如果尚有空闲的16位计时器，应避免使用该计时器。
- 和16位计时器一样，该计时器的COMPA、COMPB和OVF中断都可用，没有被内置函数占用。
## ARDUINO_ARCH_SAM
本架构部分代码参考[DueTimer](https://github.com/ivanseidel/DueTimer)。此架构有11个计时器可用：
### TOFA_TIMER0~8
`PeripheralTimers[]`，周边计时器，这些是绝大多数情况下应尽可能使用的计时器，具有32位计数和84㎒精度，是最通用、精确且无副作用的计时器
### TOFA_REALTIMER
`RealTimer`，实时计时器，具有32位计数和29.4㎑精度。较长的计时周期（跨天级别）中，通常对精度要求不高，使用此计时器可以降低功耗。由于人耳最高可听到约达20㎑的声音，与此计时器精度接近，在高音频段能够听出明显的音调偏差，一般不宜将此计时器用于产生音频。
### TOFA_SYSTIMER
`SystemTimer`，系统计时器，具有24位计数和84㎒精度。但是，此计时器为一些内置函数如`millis();delay();micros();`等依赖，使用系统计时器后再调用这些内置函数将产生未定义行为。一般应避免使用系统计时器。
# 使用入门
通过宏定义指定要使用的硬件计时器后，如果不关心具体哪个任务使用哪个计时器，可以使用自动分配，获取一个通用的`TimerClass`指针（或具有自动释放功能的等效`unique_ptr`），而无需关心硬件实现细节：
```C++
// 分配一个接受自动分配的计时器。如果没有这样的计时器，返回nullptr。此方法优先分配序数较大的的计时器（对SAM架构，优后分配实时计时器和系统计时器）。在进入setup之前调用此方法是未定义行为。被此方法分配返回的计时器将不再接受自动分配，需要手动设置Allocatable才能使其重新接受自动分配。
TimerClass const* AllocateTimer();
// 分配一个接受自动分配的计时器unique_ptr。如果没有这样的计时器，返回指针值为nullptr。此方法优先分配序数较大的的计时器（对SAM架构，优后分配实时计时器和系统计时器）。在进入setup之前调用此方法是未定义行为。此unique_ptr析构前计时器将不再接受自动分配。
inline std::unique_ptr<TimerClass const, void (*)(TimerClass const*)> AllocateTimerUnique();
```
指针指向一个具体的硬件计时器，调用`TimerClass`的成员方法布置任何计时任务。包括：
```C++
// 暂停计时器，使其相关功能暂时失效，但可以用Continue恢复，不会将计时器设为空闲。暂停一个已暂停的计时器将不做任何事。
void Pause() const;
// 继续计时器。继续一个非处于暂停状态的计时器将不做任何事
void Continue() const;
// 指示当前计时器是否接受自动分配。接受分配的计时器不一定空闲，应以Busy的返回值为准
bool Allocatable() const;
// 设置当前计时器是否接受自动分配。此项设置不会改变即使器的忙闲状态。
void Allocatable(bool A) const;
// 检查计时器是否忙碌。暂停的计时器也属于忙碌。忙碌的计时器也可能被自动分配，应以Allocatable的返回值为准
bool Busy() const;
// 终止计时器并设为空闲。一旦终止，任务将不能恢复。此操作不会改变计时器是否接受自动分配的状态。如果需要用新任务覆盖计时器上正在执行的其它任务，可以直接布置那个任务而无需先Stop。Stop确保上次布置任务的中断处理代码不再被执行，但不会还原已被任务修改的全局状态（如全局变量、引脚电平等）。
void Stop() const;
// 开始计时任务。稍后可用GetTiming获取已记录的时间。
void StartTiming() const;
// 获取从上次调用StartTiming以来经历的时间，排除中间暂停的时间。模板参数指定要返回的std::chrono::duration时间格式。如果上次StartTiming之后还调用了Stop或布置了其它任务，此方法将产生未定义行为。
template <typename T>
T GetTiming() const;
// 阻塞Duration时长。此方法一定会覆盖计时器的上一个任务，即使时长为0。此方法只能在主线程中使用，在中断处理函数中使用可能会永不返回。
template <typename T>
void Delay(T Duration) const;
// 在After时间后执行Do。不同于Delay，此方法不会阻塞当前线程，而是在指定时间后发起新的中断线程来执行任务。此方法一定会覆盖计时器的上一个任务，即使延时为0。
template <typename T>
void DoAfter(T After, std::move_only_function<void() const>&& Do) const;
// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。可选额外指定重复次数（默认无限重复）和所有重复结束后立即执行的回调。如果重复次数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
template <typename T>
void RepeatEvery(T Every, std::move_only_function<void() const>&& Do, uint64_t RepeatTimes = InfiniteRepeat, std::move_only_function<void() const>&& DoneCallback = []() {}) const;
// 每隔指定时间就重复执行任务，第一次执行也在指定时间之后。在指定的持续时间结束后执行回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
template <typename T>
void RepeatEvery(T Every, std::move_only_function<void() const>&& Do, T RepeatDuration, std::move_only_function<void() const>&& DoneCallback = nullptr) const;
// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定半周期数（即NumHalfPeriods为DoA和DoB被执行的次数之和，如果指定为奇数则DoA会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果重复半周期数为0，此方法立即执行DoneCallback，不会覆盖计时器的上一个任务。
template <typename T>
void DoubleRepeat(T AfterA, std::move_only_function<void() const>&& DoA, T AfterB, std::move_only_function<void() const>&& DoB, uint64_t NumHalfPeriods = InfiniteRepeat, std::move_only_function<void() const>&& DoneCallback = []() {}) const;
// 先在AfterA之后DoA，再在AfterB之后DoB，如此循环指定时长（时间到后立即停止，因此DoA可能会比DoB多执行一次）。所有循环完毕后，可选执行一个回调。如果指定了DoneCallback，一定会覆盖计时器的上一个任务，即使持续时间为0。
template <typename T>
void DoubleRepeat(T AfterA, std::move_only_function<void() const>&& DoA, T AfterB, std::move_only_function<void() const>&& DoB, T RepeatDuration, std::move_only_function<void() const>&& DoneCallback = nullptr) const;
```
注意，所有输入的move_only_function对象都会被移动构造而转移所有权，原对象将失效。对象直到被新任务覆盖前都不会自动析构，拥有的资源不会释放。如果这不是预期的行为，应当将资源的引用而非所有权交给move_only_function。

任务结束后，一般应当释放计时器，使其再次接受自动分配。但若使用unique_ptr则可以借助RAII机制自动释放计时器，而无需手动管理。
```C++
TimerClass const* Timer = AllocateTimer();
Timer->Delay(std::chrono::seconds(3));
Timer->Allocatable(true);
{
	std::unique_ptr<TimerClass const, void (*)(TimerClass const*)> TimerUnique = AllocateTimerUnique();
	TimerUnique->Delay(std::chrono::seconds(3));
	//TimerUnique析构时自动释放计时器
}
```
如果有些计时器需要自定义的分配方法，也可以使用Allocatable(false)禁止自动分配。