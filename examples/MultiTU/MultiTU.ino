//此示例展示如何在多翻译单元链接的场景下正确使用本库

//使用一个单独的头文件声明要使用的计时器宏
#include "Timers_one_for_all.hpp"

using namespace Timers_one_for_all;
using namespace std::chrono_literals;
TimerClass *Timer;
TimerClass *Timer2;
void setup() {
  Serial.begin(9600);
  (Timer = AllocateTimer())->StartTiming();
  Timer2 = AllocateTimer();
  pinMode(6, OUTPUT);
  Timer2->DoAfter(20s, []() {
    digitalWrite(6, HIGH);
  });
}

//SAM架构的bug，loop函数不能放在其它翻译单元
void loop2();

void loop() {
  loop2();
}
//在不多不少正好一个翻译单元中包含定义头文件
#include <TimersOneForAll_Define.hpp>