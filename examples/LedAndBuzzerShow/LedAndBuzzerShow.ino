// 此头文件非本库所有，需要自行搜索安装。本库功能本身不依赖此库，仅用于示例。
#include <Quick_digital_IO_interrupt.hpp>

// 上传该示例之前，请在7号口连接一个LED灯，8号口连接一个无源蜂鸣器的IO端口

//使用一个单独的头文件声明要使用的计时器宏
#include "Timers_one_for_all.hpp"

using namespace Timers_one_for_all;
using namespace std::chrono_literals;
constexpr uint8_t LED = 8;
constexpr uint8_t Buzzer = 22;
TimerClass* BeatTimer;
TimerClass* PauseTimer;
void setup() {
  // 首先点亮LED
  pinMode(LED, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  Serial.begin(9600);
  digitalWrite(LED, HIGH);

  // 5秒后熄灭LED灯，但不阻断程序
  TimerClass* const LEDTimer = AllocateTimer();
  LEDTimer->DoAfter(5s, []() {
    digitalWrite(LED, LOW);
  });
  //每隔2秒，就生成2500㎐脉冲1秒，重复3次
  BeatTimer = AllocateTimer();
  TimerClass* const ToneTimer = AllocateTimer();
  BeatTimer->RepeatEvery(
    2s, [ToneTimer]() {
      ToneTimer->RepeatEvery(200us, Quick_digital_IO_interrupt::DigitalToggle<Buzzer>, 1000000us);
    },
    3);
  TimerClass* const DelayTimer = AllocateTimer();

  DelayTimer->Delay(8s);
  // 将程序阻断8秒，阻断期间之前设置的中断仍然有效。阻断期间应当观察到，5秒后LED熄灭，蜂鸣器从第2s开始，5000㎐响1s停1s，重复3次。

  // 使用AllocateTimer分配的计时器，用完后记得释放才能被再次分配
  BeatTimer->Allocatable = true;
  ToneTimer->Allocatable = true;

  // 将LED灯先亮2秒，再熄灭1秒，无限循环。之前的LEDTimer可以重复使用，不必重新分配。
  digitalWrite(LED, HIGH);
  LEDTimer->DoubleRepeat(2s, Quick_digital_IO_interrupt::DigitalWrite<LED, LOW>, 1s, Quick_digital_IO_interrupt::DigitalWrite<LED, HIGH>, InfiniteRepeat);

  // 设置10秒后暂停LED的无限闪烁，暂停期间LED应当停留在亮状态
  PauseTimer = AllocateTimer();
  PauseTimer->DoAfter(10s, [LEDTimer]() {
    LEDTimer->Pause();
  });

  // 设置20秒后继续LED的无限闪烁
  TimerClass* const ContinueTimer = AllocateTimer();
  ContinueTimer->DoAfter(20s, [LEDTimer]() {
    LEDTimer->Continue();
  });

  // 设置30秒后终止LED的无限闪烁，应当停留在暗状态
  TimerClass* const StopTimer = AllocateTimer();
  StopTimer->DoAfter(30s, [LEDTimer]() {
    LEDTimer->Stop();
  });
  return;
}
void loop() {
  //等待，观察。在此期间应当看到，LED以亮2s、暗1s的周期闪烁10秒，然后卡在亮状态暂停10秒，然后继续闪烁10秒，最后彻底停止。
}

//在不多不少正好一个翻译单元中包含定义头文件
#include <TimersOneForAll_Define.hpp>