#include <Timers_one_for_all.hpp>
// 上传前在8号引脚连接LED灯
#include <Low_level_quick_digital_IO.hpp>
constexpr uint8_t LED = 8;
using namespace Timers_one_for_all;
void Toggle(const TimerClass* LEDTimer) {
  //递归实现无限闪烁
  Low_level_quick_digital_IO::DigitalToggle<LED>();
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