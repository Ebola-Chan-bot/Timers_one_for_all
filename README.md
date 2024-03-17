[README.en.md](README.en.md)

充分利用开发板上所有的计时器。

音响、方波、延迟任务、定时重复，这些任务都需要应用开发板上的计时器才能完成。有时你甚至需要多个计时器同步运行，实现多线程任务。但是，当前Arduino社区并没有提供比较完善的计时器运行库。它们能够执行的任务模式非常有限，而且用户无法指定具体要使用哪个计时器。其结果就是，经常有一些使用计时器的库发生冲突，或者和用户自己的应用发生冲突。本项目旨在将计时器可能需要使用的所有功能在所有计时器上实现，最关键的是允许用户手动指定要使用的硬件计时器，避免冲突。

本库暂时仅支持1㎳~1min范围的计时。为了提高性能，部分参数为模板参数，不能在运行时动态设置。如有超出此范围的需求，可以提Issue或 Pull request。
# 硬件计时器
如果使用多个计时器相关的库，则可能会发生计时器冲突。为了避免冲突，本库要求用户必须手动指定要在整个Sketch中使用的所有硬件计时器，方法是在.ino主代码文件中列出要使用的硬件计时器对应的预定义宏。例如：
```C++
#include <Timers_one_for_all.hpp>
TOFA_USE_TIMER0;
TOFA_USE_TIMER3;
TOFA_USE_TIMER5;
```
上述代码表示将会用到0、3、5三个硬件计时器。用户有义务确保这些计时器不会跟所使用的其它库发生冲突。已经被其它库占用的计时器不应在这里被标记将会用到。每个预定义宏实际上预处理成了对应计时器的中断处理函数定义，因此只能使用一次，否则会发生函数重定义错误。此外，还需要指定允许自动调度的计时器，例如：
```C++
TOFA_SCHEDULABLE_TIMERS(3,5);
//指定3、5计时器作为自动调度的范围

TOFA_SCHEDULABLE_TIMERS();
//即使不使用自动调度，也必须写一个空的占位符
```
此预定义宏函数可以接受可变数目的参数。在这里指定的硬件计时器必须在上一段已标记为要使用的硬件计时器范围之内。在这里列出的计时器，可以在那些不指定计时器号的功能中被自动调度使用。未在此处列出的计时器，则必须在使用计时功能时手动指定计时器号才能用上。此预定义宏函数实际上预处理成了一个全局数组变量定义，因此只能使用一次，否则会发生重定义错误。自动调度比手动指定更方便使用，但有一定的性能开销，用户需要自行权衡。
## ARDUINO_ARCH_AVR
此架构编译器必须启用C++17。打开“%LOCALAPPDATA%\Arduino15\packages\arduino\hardware\avr\<版本号>\platform.txt”并将参数“-std=gnu++11”更改为-std=gnu++17。本架构部分代码参考[MsTimer2](https://github.com/PaulStoffregen/MsTimer2)
### 计时器0
该计时器有8位，即$2^8=256$个计时状态，支持COMPA和COMPB中断，但不支持OVF中断，因为该中断被内置函数`millis();delay();micros();`占用了。本库考虑到这个情况，用COMPA和COMPB中断同样能实现所有的计时功能，因此该计时器仍然可用，但会付出一些微妙的性能代价。因此，您仍应避免使用该计时器，除非其它计时器都处于繁忙状态。
### 计时器1、3、4、5
这些计时器都具有16位，即65536个计时状态，因此比8位计时器更精确。COMPA、COMPB和OVF三个中断都为可用。绝大多数情况下，这些计时器是您的首选。注意，3~5号计时器仅在Mega2560系列开发板中支持。
### 计时器2
该计时器也是8位，但和0号计时器有些方面不同：
- 该计时器支持7种预分频模式，而所有其它计时器都只支持5种。因此该计时器比0号略微精确一些，但仍不如16位计时器。如果尚有空闲的16位计时器，应避免使用该计时器。
- 和16位计时器一样，该计时器的COMPA、COMPB和OVF中断都可用，没有被内置函数占用。
- 该计时器被著名的MsTimer2库占用。如果您同时在使用MsTimer2库，应避免使用2号计时器，以免发生潜在的冲突问题。
## ARDUINO_ARCH_SAM
此架构包含1个主计时器和9个副计时器。主计时器不能触发中断，由内置函数使用，本库使用那9个含有中断功能的副计时器。但是`DelayTask`和`TimingTask`除外，这些功能因为不需要用中断实现，所以仅使用主计时器，不允许用户手动指定计时器，也不会与其它库或内置函数冲突。所有9个副计时器都是32位。本架构部分代码参考[DueTimer](https://github.com/ivanseidel/DueTimer)
# 使用入门
根据计时任务类型的不同，可能需要执行最多三个步骤以实现预期功能：获取`HardwareTimer`、创建任务、执行任务。
## 获取`HardwareTimer`
几乎所有任务的创建都需要先获取`HardwareTimer`，例外是SAM架构上的`DelayTask`和`TimingTask`，它们使用独立的主计时器实现而不需要`HardwareTimer`。

你可以选择手动指定`HardwareTimer`编号，或者让本库自动为你分配。