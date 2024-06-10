// 上传该示例之前，请在7号口连接一个LED灯，8号口连接一个无源蜂鸣器的IO端口
#include <Timers_one_for_all.hpp>
using namespace Timers_one_for_all;
constexpr uint8_t LED = 7;
constexpr uint8_t Buzzer = 8;
void LightDown()
{
  digitalWrite(LED, LOW);
}
void setup()
{
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.begin(9600);
  digitalWrite(LED, HIGH);
  // 5秒后熄灭LED灯，但不阻断程序
  const TimerClass*const LEDTimer=AllocateTimer();
  LEDTimer->DoAfter(std::chrono::seconds(5), []()
                               { digitalWrite(LED, LOW); });
  const TimerClass*const BeatTimer=AllocateTimer();
  const TimerClass*const ToneTimer=AllocateTimer();
  BeatTimer->RepeatEvery(std::chrono::seconds(2),)
  // 设置4号计时器，每隔2秒，就用5号计时器生成2000㎐脉冲1秒，重复3次
  RepeatAfter<4, 2000, 3>(PlayTone<5, Buzzer, 2000, 1000>);
  // 设置计时器1，将程序阻断7秒
  Delay<1, 7000>();
  // 设置计时器1，将LED灯先亮2秒，再熄灭1秒，无限循环
  SquareWave<1, LED, 2000, 1000>();
  // 设置计时器3在8秒后停止计时器1
  DoAfter<3, 8000>(ShutDown<1>);
  // 观察到，LED灯明暗循环两次后，最终停在了亮状态，因为1号计时器尚未触发暗事件就被停止了
  // 亮1秒暗2秒的方波，重复5个循环
  Delay<4, 8000>();
  SquareWave<1, LED, 1000, 2000, 5>();
}
void loop()
{
}