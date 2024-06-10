#include <Timers_one_for_all.hpp>
#include <iostream>
using namespace Timers_one_for_all;
static const TimerClass *const Timer = AllocateTimer();
void setup()
{
	Serial.begin(9600);
	Timer->StartTiming();
}
void loop()
{
	std::cout << "按Enter显示已经运行了几秒：";
	std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
	std::cin.get();
	std::cout << "已经过" << Timer->GetTiming<std::chrono::seconds>().count() << "秒\n";
}
