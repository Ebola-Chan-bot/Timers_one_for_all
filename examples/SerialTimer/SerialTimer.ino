
void setup() {
#if defined ARDUINO_ARCH_AVR
#define TOFA_NumTimers defined ARDUINO_ARCH_AVR
#endif
	constexpr uint8_t NumTimers = TOFA_NumTimers;
}

void loop() 
{

}
