#pragma once
// 如果本库与其它库发生计时器冲突，可以在这里注释掉一些宏定义，对应的硬件计时器将不再被本库使用。
#define TOFA_USE_TIMER0
#define TOFA_USE_TIMER1
#define TOFA_USE_TIMER2
#define TOFA_USE_TIMER3
#define TOFA_USE_TIMER4
#define TOFA_USE_TIMER5
#ifdef ARDUINO_ARCH_SAM
// 只有SAM平台支持6~8号计时器
#define TOFA_USE_TIMER6
#define TOFA_USE_TIMER7
#define TOFA_USE_TIMER8
#endif