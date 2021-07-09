
充分利用开发板上所有的计时器。

音响、方波、延迟任务、定时重复，这些任务都需要应用开发板上的计时器才能完成。有时你甚至需要多个计时器同步运行，实现多线程任务。但是，当前Arduino社区并没有提供比较完善的计时器运行库。它们能够执行的任务模式非常有限，而且用户无法指定具体要使用哪个计时器。其结果就是，经常有一些使用计时器的库发生冲突，或者和用户自己的应用发生冲突。本项目旨在将计时器可能需要使用的所有功能在所有计时器上实现，最关键的是允许用户手动指定要是用的硬件计时器，避免冲突。

**由于作者精力有限，目前本项目所有函数，仅时长、重复次数参数可以在运行时动态设置，其它参数必须为编译期常量，作为模板参数。后续有空会实现更多可以动态设置参数的API。当然更欢迎上GitHub主页贡献Pull request。**
# 硬件计时器
本库所有公开API的第一个模板参数，都是计时器的编号。不同计时器之间完全独立运行，互不干扰。用户必须在代码中手动指定要使用哪个硬件计时器来执行功能。不同CPU中的计时器数目和属性各不相同，此处以Arduino Mega开发板上常用的ATMega2560系列CPU为例：
## 计时器0
该计时器有8位，即2^8=256个计时状态，支持COMPA和COMPB中断，但不支持OVF中断，因为该中断被内置函数`millis();delay();micros();`占用了。本库考虑到这个情况，用COMPA和COMPB中断同样能实现所有的计时功能，因此该计时器仍然可用，但会付出一些微妙的性能代价。因此，您仍应避免使用该计时器，除非其它计时器都处于繁忙状态。
## 计时器1、3、4、5
这些计时器都具有16位，即65536个计时状态，因此比8位计时器更精确。COMPA、COMPB和OVF三个中断都为可用。绝大多数情况下，这些计时器是您的首选。
## 计时器2
该计时器也是8位，但和0号计时器有些方面不同：
- 该计时器支持7种预分频模式，而所有其它计时器都只支持5种。因此该计时器比0号略微精确一些，但仍不如16位计时器。如果尚有空闲的16位计时器，应避免使用该计时器。
- 和16位计时器一样，该计时器的COMPA、COMPB和OVF中断都可用，没有被内置函数占用。
- 该计时器被著名的MsTimer2库占用。如果您同时在使用MsTimer2库，应避免使用2号计时器，以免发生潜在的冲突问题。

再次重申，上述硬件说明仅适用于ATMega2560系列CPU，本库对该系列CPU也具有最好的支持。请确认您的开发板搭载的CPU型号及计时器硬件属性问题。此外，内置函数`analogWrite()`和`tone()`会在运行时动态地征用某些计时器，并且不会检测该计时器是否正在被本库使用；本库也无法检测用户指定的计时器是否正在被这些内置函数使用。因此如果同时使用这些内置函数，您将可能需要考虑潜在的冲突问题。

如果你使用的开发板并非ATMega2560系列的CPU，也可能可以使用本库。具体需要参考你的CPU数据表，看是否具有兼容的硬件计时器。

Make full use of all your hardware timers on your Arduino board. 

The only library you can choose any hardware timer you like to use in your timing function. Tones, square waves, delayed tasks, timed repetitive tasks are provided as building blocks for your own sophisticated multi-timer tasks. My library hides hardware register details for you.

**Currently, for all APIs, only time length and RepeatTimes arguments can be specified at runtime. Other arguments are all template arguments, i.e., they must be known as constexprs at compiling time. I'll implement more runtime arguments if I have more free time. Pull requests are fully welcomed on my GitHub site.**
# Timers
All public APIs are under namespace TimersOneForAll, and require TimerCode as the first template argument. TimerCode indicates which hardware timer you want to use. Hardware timers vary by CPUs. For ATMega2560 (Arduino Mega), there're 6 timers:
## Timer 0
This timer is 8-bit, which means it has 2^8=256 states. It can generate COMPA and COMPB interrupts, but not OVF, because it's occupied by Arduino builtin function `millis();delay();micros();`. This means that, though functionally usable (I specially workarounded this issue in my code), this timer is the last one you want to use among all the timers. Only use it if other timers are really busy.
## Timer 1, 3, 4, 5
These timers are all 16-bit, with 65536 states, which means that they're more accurate than 8-bit timers. COMPA, COMPB and OVF interrupts are all available. You may want to use these timers for most scenarios.
## Timer 2
This timer is also 8-bit, but different from timer 0 at 3 aspects:
- It has 7 prescalers, which is the most among all timers - all other timers have 5 prescalers available. This gives timer 2 slightly more accuracy than timer 0, but still less accurate than 16-bit timers. Avoid using it if you still have free 16-bit timers.
- It has 3 interrupts available, COMPA, COMPB and OVF, just like 16-bit timers. None of them is occupied by builtin functions.
- It's used by a famous timer library MsTimer2. You can't use this timer if you have MsTimer2 included.

Moreover, builtin `analogWrite()` and `tone()` may dynamically occupy any of the timers listed above. You'll have to handle potential conflicts if you want to use these builtins.

If you aren't using ATMega2560 CPU series, you may or may not be able to use this library. Refer to your datasheet to see if it has compatible hardware timer configurations.
# API参考 API Reference
```C++
//在指定的毫秒数后触发一个计时器中断，调用你的函数。
//Call your function with a timer interrupt after given milliseconds
template <uint8_t TimerCode, 
uint16_t AfterMilliseconds,//If set to 0, DoTask will be called directly in this function call. Hardware timer won't be disturbed.
void (*DoTask)()>
void DoAfter();
//允许运行时动态设置毫秒数
//Specify milliseconds at runtime
template <uint8_t TimerCode, void (*DoTask)()>
void DoAfter(uint16_t AfterMilliseconds);

//每隔指定的毫秒数，无限重复调用你的函数。第一次调用也将在那个毫秒数之后发生。
//Repetitively and infinitely call your function with a timer interrupt for each IntervalMilliseconds. The first interrupt happens after IntervalMilliseconds, too.
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)()>
void RepeatAfter();
//仅重复有限次数，重复全部结束后触发DoneCallback回调
//Repeat for only RepeatTimes. After all repeats done, DoneCallback is called.
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), int32_t RepeatTimes, void (*DoneCallback)() = nullptr>
void RepeatAfter();
//允许运行时动态设置毫秒数。重复次数不指定的话则为无限重复。重复全部结束后触发DoneCallback回调
//Specify milliseconds at runtime. After all repeats done, DoneCallback is called.
template <uint8_t TimerCode, void (*DoTask)(), int32_t RepeatTimes, void (*DoneCallback)() = nullptr>
void RepeatAfter(uint16_t IntervalMilliseconds);
//每隔指定毫秒数重复执行任务。重复次数若为负数，或不指定重复次数，则默认无限重复
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), void (*DoneCallback)() = nullptr>
void RepeatAfter(int32_t RepeatTimes);

//将当前时刻设为0，计量经过的毫秒数。读取MillisecondsElapsed变量来获得经过的毫秒数。
//Set the time now as 0 and start to record time elapsed. Read MillisecondsElapsed variable to get the time elapsed.
template <uint8_t TimerCode>
void StartTiming();
//获取自上次调用StartTiming以来所经过的毫秒数。
//Get MillisecondsElapsed after the last call of StartTiming.
template <uint8_t TimerCode>
static volatile uint16_t MillisecondsElapsed;
//设为false可以暂停计时，重新设为true可继续计时
//Set to false can freeze MillisecondsElapsed from being ticked by the timer (lock the variable only, timer still running). Set to true to continue timing.
template <uint8_t TimerCode>
static volatile bool Running;

//在指定引脚上无限播放指定频率的音调
//Play a tone of FrequencyHz on PinCode endlessly.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
void PlayTone();
//只播放限定的毫秒数。播放结束后触发DoneCallback回调
//Play for only given Milliseconds. After the tone ended, DoneCallback is called.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds, void (*DoneCallback)() = nullptr>
void PlayTone();
//允许运行时动态设置毫秒数。播放结束后触发DoneCallback回调
//Specify milliseconds at runtime. After the tone ended, DoneCallback is called.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, void (*DoneCallback)() = nullptr>
void PlayTone(uint16_t Milliseconds);

//在引脚上生成无限循环的方波。不同于音调，这里可以指定高电平和低电平的不同时长
//Generate an infinite sequence of square wave. The high level and low level can have different time length.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWave();
//仅生成有限个周期数的方波。周期全部结束后触发DoneCallback回调
//Generate the square wave for only RepeatTimes cycles. After all cycles done, DoneCallback is called.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int16_t RepeatTimes, void (*DoneCallback)() = nullptr>
void SquareWave();
//允许运行时动态设置毫秒数。重复次数不指定的话则为无限重复。周期全部结束后触发DoneCallback回调
//Specify milliseconds at runtime. After all cycles done, DoneCallback is called.
template <uint8_t TimerCode, uint8_t PinCode, int16_t RepeatTimes, void (*DoneCallback)() = nullptr>
void SquareWave(uint16_t HighMilliseconds, uint16_t LowMilliseconds);
//允许运行时动态设置重复次数。重复次数不指定的话则为无限重复。
//Specify RepeatTimes at runtime. Infinitely repeat if not specified.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, void (*DoneCallback)() = nullptr>
void SquareWave(int16_t RepeatTimes);

//阻塞当前代码执行指定毫秒数
//Block current code from running for DelayMilliseconds
template <uint8_t TimerCode, uint16_t DelayMilliseconds>
void Delay();
//允许运行时动态设置毫秒数
//Specify milliseconds at runtime
template <uint8_t TimerCode>
void Delay(uint16_t DelayMilliseconds);

//取消指定给特定计时器的所有任务。其它计时器不受影响。
//Abort all tasks assigned to TimerCode. Other timers won't be affected.
template <uint8_t TimerCode>
void ShutDown();
```
