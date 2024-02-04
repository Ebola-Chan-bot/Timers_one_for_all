#include <Timers_one_for_all.hpp>
using namespace Timers_one_for_all;
//上传该示例之前，请确保你的PC准备好收发串口信号
void setup()
{
  constexpr auto TI=TimerInfo[1].CounterMax;
}
void loop()
{
}
//上传后，可以从PC上向串口发送任意单字节，每次发送都会收到一个16位整数，指示经过的毫秒数。达到65535毫秒后会归零。