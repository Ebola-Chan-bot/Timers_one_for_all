#include <TimersOneForAll.h>
using namespace TimersOneForAll;
//上传该示例之前，请确保你的PC准备好收发串口信号
void setup()
{
	Serial.begin(9600);
	//使用1号计时器开始计时
	StartTiming<1>();
	Serial.setTimeout(-1);
}
void loop()
{
	static uint8_t Instruction = 0;
	Serial.readBytes(&Instruction, 1);
	Serial.write((uint8_t*)&MillisecondsElapsed<1>, 2);
}
//上传后，可以从PC上向串口发送任意单字节，每次发送都会受到一个16位整数，指示经过的毫秒数。达到65535毫秒后会归零。