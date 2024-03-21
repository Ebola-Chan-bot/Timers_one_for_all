#pragma once
// 如果本库与其它库发生计时器冲突，可以在这里注释掉一些宏定义，对应的硬件计时器将不再被本库使用。
// 不同计时器的使用会干扰不同引脚上的analogWrite，请查阅文档确认
#define TOFA_USE_TIMER0
#define TOFA_USE_TIMER1
#define TOFA_USE_TIMER2
#define TOFA_USE_TIMER3
#define TOFA_USE_TIMER4
#define TOFA_USE_TIMER5
#ifdef ARDUINO_ARCH_SAM
#define TOFA_USE_TIMER6
#define TOFA_USE_TIMER7
#define TOFA_USE_TIMER8
#define TOFA_USE_SYSTICK
#endif
#ifdef ARDUINO_ARCH_AVR
#ifdef TOFA_USE_TIMER0
#warning 使用计时器0可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_USE_TIMER0
#endif
#ifdef TOFA_USE_TIMER2
#warning 使用计时器2可能导致内置tone等音调相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_USE_TIMER2
#endif
#endif
#ifdef ARDUINO_ARCH_SAM
#ifdef TOFA_USE_SYSTICK
#warning 使用系统计时器可能导致内置 micros millis delay 等时间相关函数工作异常。如果不需要使用这些内置函数，可以忽略此警告；否则考虑取消宏定义TOFA_USE_SYSTICK
#endif
#endif