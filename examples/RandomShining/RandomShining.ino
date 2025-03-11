//使用一个单独的头文件声明要使用的计时器宏
#include "Timers_one_for_all.hpp"

constexpr uint8_t LED = 8;
using namespace Timers_one_for_all;
void Toggle(TimerClass* LEDTimer) {
  //递归实现无限闪烁
  static uint8_t State = LOW;
  digitalWrite(LED, State = 1 - State);
  LEDTimer->DoAfter(std::chrono::milliseconds(random(2000)), [LEDTimer]() {
    Toggle(LEDTimer);
  });
}
void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  randomSeed(analogRead(6));
  Toggle(AllocateTimer());
}
void loop() {
  //等待，查看闪烁效果
}

//在不多不少正好一个翻译单元中包含定义头文件
#include <TimersOneForAll_Define.hpp>