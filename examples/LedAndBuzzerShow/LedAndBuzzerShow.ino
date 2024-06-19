// 上传该示例之前，请在7号口连接一个LED灯，8号口连接一个无源蜂鸣器的IO端口
#include <Timers_one_for_all.hpp>
// 本库经常搭配低级快速数字读写库一起使用
#include <Low_level_quick_digital_IO.hpp>
using namespace Timers_one_for_all;
constexpr uint8_t LED = 7;
constexpr uint8_t Buzzer = 8;
const TimerClass *BeatTimer;
const TimerClass *PauseTimer;
void setup() {
  // 首先点亮LED
  pinMode(LED, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(9600);

  // 5秒后熄灭LED灯，但不阻断程序
  const TimerClass *const LEDTimer = AllocateTimer();
  LEDTimer->DoAfter(std::chrono::seconds(5), []() {
    digitalWrite(LED, LOW);
  });
return;
  // 每隔2秒，就生成2000㎐脉冲1秒，重复3次
  BeatTimer = AllocateTimer();
  const TimerClass *const ToneTimer = AllocateTimer();
  BeatTimer->RepeatEvery(
    std::chrono::seconds(2), [ToneTimer]() {
      ToneTimer->RepeatEvery<std::chrono::microseconds>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1)) / 2000, Low_level_quick_digital_IO::DigitalToggle<Buzzer>, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1)));
    },
    3);
  // 将程序阻断7秒，阻断期间之前设置的中断仍然有效。阻断期间应当观察到，5秒后LED熄灭，蜂鸣器每隔2s以2000㎐响1s，重复3次。
  const TimerClass *const DelayTimer = AllocateTimer();
  DelayTimer->Delay(std::chrono::seconds(7));
  // 使用AllocateTimer分配的计时器，用完后记得释放才能被再次分配
  BeatTimer->Allocatable(true);
  ToneTimer->Allocatable(true);

  // 将LED灯先亮2秒，再熄灭1秒，无限循环。之前的LEDTimer可以重复使用，不必重新分配。
  digitalWrite(LED, HIGH);
  LEDTimer->DoubleRepeat(std::chrono::seconds(2), Low_level_quick_digital_IO::DigitalWrite<LED, LOW>, std::chrono::seconds(1), Low_level_quick_digital_IO::DigitalWrite<LED, HIGH>, InfiniteRepeat);

  // 设置10秒后暂停LED的无限闪烁
  PauseTimer = AllocateTimer();
  PauseTimer->DoAfter(std::chrono::seconds(10), [LEDTimer]() {
    LEDTimer->Pause();
  });

  // 设置20秒后继续LED的无限闪烁
  const TimerClass *const ContinueTimer = AllocateTimer();
  ContinueTimer->DoAfter(std::chrono::seconds(20), [LEDTimer]() {
    LEDTimer->Continue();
  });

  // 设置30秒后终止LED的无限闪烁
  const TimerClass *const StopTimer = AllocateTimer();
  StopTimer->DoAfter(std::chrono::seconds(30), [LEDTimer]() {
    LEDTimer->Stop();
  });
}
void loop() {
  //等待，观察。在此期间应当看到，LED以亮2s、暗1s的周期闪烁10秒，然后卡在亮状态暂停10秒，然后继续闪烁10秒，最后彻底停止。
  //Serial.println(SysTick->VAL);
}