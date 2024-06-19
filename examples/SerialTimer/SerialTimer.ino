#include <Timers_one_for_all.hpp>
#include <iostream>
using namespace Timers_one_for_all;
static const TimerClass *Timer;
void setup() {
  Serial.begin(9600);
  (Timer = &HardwareTimer0)->StartTiming();
}
void loop() {
  std::cout << "输入任意字符，按Enter显示已经运行了几秒：";
  std::cin.get();
  std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
  std::cout << "已经过" << Timer->GetTiming<std::chrono::seconds>().count() << "秒\n";
}
