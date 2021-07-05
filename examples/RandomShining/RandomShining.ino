#include <TimersOneForAll.h>
//上传前在7号和8号引脚连接LED灯
constexpr uint8_t LED1 = 7;
constexpr uint8_t LED2 = 8;
void setup()
{
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	digitalWrite(LED1, HIGH);
	//在8号引脚的LED灯上生成方波，高低电平时长动态随机生成
	randomSeed(analogRead(6));
	TimersOneForAll::SquareWave<3, LED2>(random(5000), random(5000));
}
void loop()
{
	static bool CurrentState = HIGH;
	//在7号引脚的LED灯上生成即时随机的闪烁，每次都重新随机
	TimersOneForAll::Delay<1>(random(10000));
	digitalWrite(LED1, CurrentState = CurrentState ? LOW : HIGH);
}