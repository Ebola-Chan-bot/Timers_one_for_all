#include <Arduino.h>
void SerialWrite(uint8_t Value)
{
	Serial.write(Value);
}
void SerialWrite(uint16_t Value)
{
	Serial.write((uint8_t *)&Value, 2);
}
void SerialWrite(uint32_t Value)
{
	Serial.write((uint8_t *)&Value, 4);
}