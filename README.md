Make full use of all your hardware timers on your Arduino board. 

The only library you can choose any hardware timer you like to use in your timing function. Tones, square waves, delayed tasks, timed repetitive tasks are provided as building blocks for your own sophisticated multi-timer tasks. My library hides hardware register details for you.

Currently all APIs are function templates, which means all arguments must be known as constants at compile-time. Non-template versions will be gradually added if I have more free time. However, you're welcomed to modify my code - not so difficult to understand if you are familiar with ATMega registers.
# Timers
All public APIs are under namespace TimersOneForAll, and require TimerCode as the first template argument. TimerCode indicates which hardware timer you want to use. Hardware timers vary by CPUs. For ATMega2560, there're 6 timers:
## Timer 0
This timer is 8-bit, which means it has 2^8=256 states. It can generate COMPA and COMPB interrupts, but not OVF, because it's occupied by Arduino builtin function `millis()`. This means this timer is the last one you want to use among all the timers. Only use it if you really have strong reasons.
## Timer 1, 3, 4, 5
These timers are all 16-bit, with 65536 states, which means that they're more accurate than 8-bit timers. COMPA, COMPB and OVF interrupts are all available. You may want to use these timers for most scenarios.
## Timer 2
This timer is also 8-bit, but different from timer 0 at 3 aspects:
- It has 7 prescalers, which is the most among all timers - all other timers have 5 prescalers available. This gives timer 2 slightly more accuracy than timer 0, but still less accurate than 16-bit timers. Avoid using it if you still have free 16-bit timers.
- It has 3 interrupts available, COMPA, COMPB and OVF, just like 16-bit timers. None of them is occupied by builtin functions.
- It's used by a famous timer library MsTimer2. You can't use this timer if you have MsTimer2 included.

Moreover, builtin `analogWrite()` and `tone()` may dynamically occupy any of the timers listed above. You'll have to handle potential conflicts if you want to use these builtins.
# API reference
```C++
//Call your function with a timer interrupt after given milliseconds
template <uint8_t TimerCode, 
uint16_t AfterMilliseconds,//If set to 0, DoTask will be called directly in this function call. Hardware timer won't be disturbed.
void (*DoTask)()>
void DoAfter()

//Repetitively and infinitely call your function with a timer interrupt for each IntervalMilliseconds. The first interrupt happens after IntervalMilliseconds, too.
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)()>
void RepeatAfter()

//Repeat for only RepeatTimes
template <uint8_t TimerCode, uint16_t IntervalMilliseconds, void (*DoTask)(), uint32_t RepeatTimes>
void RepeatAfter()

//Set the time now as 0 and start to record time elapsed. Read MillisecondsElapsed variable to get the time elapsed.
template <uint8_t TimerCode>
void StartTiming()

//Get MillisecondsElapsed after the last call of StartTiming.
template <uint8_t TimerCode>
static volatile uint16_t MillisecondsElapsed;

//Play a tone of FrequencyHz on PinCode endlessly.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz>
void PlayTone()

//Play for only given Milliseconds
template <uint8_t TimerCode, uint8_t PinCode, uint16_t FrequencyHz, uint16_t Milliseconds>
void PlayTone()

//Generate an infinite sequence of square wave. The high level and low level can have different time length.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds>
void SquareWave()

//Generate the square wave for only RepeatTimes cycles.
template <uint8_t TimerCode, uint8_t PinCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, int16_t RepeatTimes>
void SquareWave()

//Abort all tasks assigned to TimerCode. Other timers won't be affected.
template <uint8_t TimerCode>
void ShutDown()
```
