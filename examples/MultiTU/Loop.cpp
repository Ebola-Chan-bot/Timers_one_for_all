//在其它翻译单元中只声明，不定义
#include "Timers_one_for_all.hpp"

extern Timers_one_for_all::TimerClass const *Timer;
#include <iostream>
void loop() {
	std::cout << "输入任意字符，按Enter显示已经运行了几秒：";
	std::cin.get();
	std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
	std::cout << "已经过" << Timer->GetTiming<std::chrono::seconds>().count() << "秒\n";
  }

  //不能包含定义头文件，因为MultiTU.ino中已经包含了定义，不能重复定义