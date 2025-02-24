#pragma once

/*
先列出要使用的所有计时器。本示例需要至少5个计时器，如果数目不足，AllocateTimer将返回nullptr，导致未定义行为
*/

//AVR和SAM架构都支持的计时器
//#define TOFA_TIMER0
#define TOFA_TIMER1
#define TOFA_TIMER2
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

/*
再包含声明头文件
*/
#include <TimersOneForAll_Declare.hpp>