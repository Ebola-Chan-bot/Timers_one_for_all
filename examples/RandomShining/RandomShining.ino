#include <Timers_one_for_all.hpp>
// 上传前在8号引脚连接LED灯
#include <Low_level_quick_digital_IO.hpp>
constexpr uint8_t LED = 8;
using namespace Timers_one_for_all;
void setup()
{
	pinMode(LED, OUTPUT);
	randomSeed(analogRead(6));
	const TimerClass *const LEDTimer = AllocateTimer();

	//通过递归捕获实现无限循环，每次抽取0~2000㎳内的一个随机间隔闪烁LED
	const std::function<void()> Do = [LEDTimer, Do]()
	{
		Low_level_quick_digital_IO::DigitalToggle<LED>();
		LEDTimer->DoAfter(std::chrono::milliseconds(random(2000)), Do);
	};
	Do();
}
void loop()
{
	//等待，查看闪烁效果
}