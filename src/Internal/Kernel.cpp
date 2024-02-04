#include "Kernel.h"
#ifdef ARDUINO_ARCH_AVR
#define TimerSpecialize(Code)                      \
	template <>                                    \
	volatile uint8_t &TCCRA<Code> = TCCR##Code##A; \
	template <>                                    \
	volatile uint8_t &TCCRB<Code> = TCCR##Code##B; \
	template <>                                    \
	volatile uint8_t &TIMSK<Code> = TIMSK##Code;   \
	template <>                                    \
	volatile uint8_t &TIFR<Code> = TIFR##Code;     \
	template <>                                    \
	uint16_t GetTCNT<Code>()                       \
	{                                              \
		return TCNT##Code;                         \
	}                                              \
	template <>                                    \
	void SetTCNT<Code>(uint16_t TCNT)              \
	{                                              \
		TCNT##Code = TCNT;                         \
	}                                              \
	template <>                                    \
	uint16_t GetOCRA<Code>()                       \
	{                                              \
		return OCR##Code##A;                       \
	}                                              \
	template <>                                    \
	void SetOCRA<Code>(uint16_t OCRA)              \
	{                                              \
		OCR##Code##A = OCRA;                       \
	}                                              \
	template <>                                    \
	uint16_t GetOCRB<Code>()                       \
	{                                              \
		return OCR##Code##B;                       \
	}                                              \
	template <>                                    \
	void SetOCRB<Code>(uint16_t OCRB)              \
	{                                              \
		OCR##Code##B = OCRB;                       \
	}                                              \
	ISR(TIMER##Code##_COMPA_vect)                  \
	{                                              \
		COMPA<Code>();                             \
	}                                              \
	ISR(TIMER##Code##_COMPB_vect)                  \
	{                                              \
		COMPB<Code>();                             \
	}
namespace Timers_one_for_all
{
	namespace Internal
	{
#if defined(TCNT0) && !defined(TOFA_DISABLE_0)
		TimerSpecialize(0);
// Timer0的溢出中断被内置millis()函数占用了，无法使用
#endif
#define TimerSpecializeIsr(Code) \
	TimerSpecialize(Code);       \
	ISR(TIMER##Code##_OVF_vect)  \
	{                            \
		OVF<Code>();             \
	}
#if defined(TCNT1) && !defined(TOFA_DISABLE_1)
		TimerSpecializeIsr(1);
#endif
#if defined(TCNT2) && !defined(TOFA_DISABLE_2)
		TimerSpecializeIsr(2);
#endif
#if defined(TCNT3) && !defined(TOFA_DISABLE_3)
		TimerSpecializeIsr(3);
#endif
#if defined(TCNT4) && !defined(TOFA_DISABLE_4)
		TimerSpecializeIsr(4);
#endif
#if defined(TCNT5) && !defined(TOFA_DISABLE_5)
		TimerSpecializeIsr(5);
#endif
	}
}
#endif
#ifdef ARDUINO_ARCH_SAM
using namespace Timers_one_for_all::Internal;
TimerSetting Timers_one_for_all::Internal::GetTimerSetting(uint16_t Milliseconds)
{
	const uint32_t Ticks = MaxPrecision * Milliseconds;
	const uint32_t Clock = u32Min(__builtin_ctz(Ticks) >> 1, 3);
	return {Clock, Ticks >> (Clock << 1)};
}
constexpr struct
{
	Tc *tc;
	uint32_t channel;
	IRQn_Type irq;
} Timers[NUM_TIMERS] = {
	{TC0, 0, TC0_IRQn},
	{TC0, 1, TC1_IRQn},
	{TC0, 2, TC2_IRQn},
	{TC1, 0, TC3_IRQn},
	{TC1, 1, TC4_IRQn},
	{TC1, 2, TC5_IRQn},
	{TC2, 0, TC6_IRQn},
	{TC2, 1, TC7_IRQn},
	{TC2, 2, TC8_IRQn},
};
void (*TC_Handlers[9])();
#ifndef TOFA_DISABLE_0
void TC0_Handler() { TC_Handlers[0](); }
#endif
#ifndef TOFA_DISABLE_1
void TC1_Handler() { TC_Handlers[1](); }
#endif
#ifndef TOFA_DISABLE_2
void TC2_Handler() { TC_Handlers[2](); }
#endif
#ifndef TOFA_DISABLE_3
void TC3_Handler() { TC_Handlers[3](); }
#endif
#ifndef TOFA_DISABLE_4
void TC4_Handler() { TC_Handlers[4](); }
#endif
#ifndef TOFA_DISABLE_5
void TC5_Handler() { TC_Handlers[5](); }
#endif
#ifndef TOFA_DISABLE_6
void TC6_Handler() { TC_Handlers[6](); }
#endif
#ifndef TOFA_DISABLE_7
void TC7_Handler() { TC_Handlers[7](); }
#endif
#ifndef TOFA_DISABLE_8
void TC8_Handler() { TC_Handlers[8](); }
#endif
void Timers_one_for_all::Internal::SetRepeater(const TimerSetting &TS, void (*Callback)(), uint8_t Timer)
{
	TC_Handlers[Timer] = Callback;
	pmc_set_writeprotect(false);
	const auto &T = Timers[Timer];
	pmc_enable_periph_clk(T.irq);

	// Set up the Timer in waveform mode which creates a PWM
	// in UP mode with automatic trigger on RC Compare
	// and sets it up with the determined internal clock as clock input.
	TC_Configure(T.tc, T.channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TS.Clock);
	// Reset counter and fire interrupt when RC value is matched:
	TC_SetRC(T.tc, T.channel, TS.RC);
	// Enable the RC Compare Interrupt...
	T.tc->TC_CHANNEL[T.channel].TC_IER = TC_IER_CPCS;
	// ... and disable all others.
	T.tc->TC_CHANNEL[T.channel].TC_IDR = ~TC_IER_CPCS;

	NVIC_ClearPendingIRQ(T.irq);
	NVIC_EnableIRQ(T.irq);

	TC_Start(T.tc, T.channel);
}
const DueTimer::Timer DueTimer::Timers[NUM_TIMERS] = {
	{TC0, 0, TC0_IRQn},
	{TC0, 1, TC1_IRQn},
	{TC0, 2, TC2_IRQn},
	{TC1, 0, TC3_IRQn},
	{TC1, 1, TC4_IRQn},
	{TC1, 2, TC5_IRQn},
	{TC2, 0, TC6_IRQn},
	{TC2, 1, TC7_IRQn},
	{TC2, 2, TC8_IRQn},
};

// Fix for compatibility with Servo library
void (*DueTimer::callbacks[NUM_TIMERS])() = {};
double DueTimer::_frequency[NUM_TIMERS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

/*
	Initializing all timers, so you can use them like this: Timer0.start();
*/
DueTimer Timer(0);

DueTimer Timer1(1);
// Fix for compatibility with Servo library
DueTimer Timer0(0);
DueTimer Timer2(2);
DueTimer Timer3(3);
DueTimer Timer4(4);
DueTimer Timer5(5);
DueTimer Timer6(6);
DueTimer Timer7(7);
DueTimer Timer8(8);

DueTimer::DueTimer(unsigned short _timer) : timer(_timer)
{
	/*
		The constructor of the class DueTimer
	*/
}

DueTimer DueTimer::getAvailable(void)
{
	/*
		Return the first timer with no callback set
	*/

	for (int i = 0; i < NUM_TIMERS; i++)
	{
		if (!callbacks[i])
			return DueTimer(i);
	}
	// Default, return Timer0;
	return DueTimer(0);
}

DueTimer &DueTimer::attachInterrupt(void (*isr)())
{
	/*
		Links the function passed as argument to the timer of the object
	*/

	callbacks[timer] = isr;

	return *this;
}

DueTimer &DueTimer::detachInterrupt(void)
{
	/*
		Links the function passed as argument to the timer of the object
	*/

	stop(); // Stop the currently running timer

	callbacks[timer] = NULL;

	return *this;
}

DueTimer &DueTimer::start(long microseconds)
{
	/*
		Start the timer
		If a period is set, then sets the period and start the timer
	*/

	if (microseconds > 0)
		setPeriod(microseconds);

	if (_frequency[timer] <= 0)
		setFrequency(1);

	NVIC_ClearPendingIRQ(Timers[timer].irq);
	NVIC_EnableIRQ(Timers[timer].irq);

	TC_Start(Timers[timer].tc, Timers[timer].channel);

	return *this;
}

DueTimer &DueTimer::stop(void)
{
	/*
		Stop the timer
	*/

	NVIC_DisableIRQ(Timers[timer].irq);

	TC_Stop(Timers[timer].tc, Timers[timer].channel);

	return *this;
}

uint8_t DueTimer::bestClock(double frequency, uint32_t &retRC)
{
	/*
		Pick the best Clock, thanks to Ogle Basil Hall!

		Timer		Definition
		TIMER_CLOCK1	MCK /  2
		TIMER_CLOCK2	MCK /  8
		TIMER_CLOCK3	MCK / 32
		TIMER_CLOCK4	MCK /128
	*/
	const struct
	{
		uint8_t flag;
		uint8_t divisor;
	} clockConfig[] = {
		{TC_CMR_TCCLKS_TIMER_CLOCK1, 2},
		{TC_CMR_TCCLKS_TIMER_CLOCK2, 8},
		{TC_CMR_TCCLKS_TIMER_CLOCK3, 32},
		{TC_CMR_TCCLKS_TIMER_CLOCK4, 128}};
	float ticks;
	float error;
	int clkId = 3;
	int bestClock = 3;
	float bestError = 9.999e99;
	do
	{
		ticks = (float)VARIANT_MCK / frequency / (float)clockConfig[clkId].divisor;
		// error = abs(ticks - round(ticks));
		error = clockConfig[clkId].divisor * abs(ticks - round(ticks)); // Error comparison needs scaling
		if (error < bestError)
		{
			bestClock = clkId;
			bestError = error;
		}
	} while (clkId-- > 0);
	ticks = (float)VARIANT_MCK / frequency / (float)clockConfig[bestClock].divisor;
	retRC = (uint32_t)round(ticks);
	return clockConfig[bestClock].flag;
}

DueTimer &DueTimer::setFrequency(double frequency)
{
	/*
		Set the timer frequency (in Hz)
	*/

	// Prevent negative frequencies
	if (frequency <= 0)
	{
		frequency = 1;
	}

	// Remember the frequency — see below how the exact frequency is reported instead
	//_frequency[timer] = frequency;

	// Get current timer configuration
	Timer t = Timers[timer];

	uint32_t rc = 0;
	uint8_t clock;

	// Tell the Power Management Controller to disable
	// the write protection of the (Timer/Counter) registers:
	pmc_set_writeprotect(false);

	// Enable clock for the timer
	pmc_enable_periph_clk((uint32_t)t.irq);

	// Find the best clock for the wanted frequency
	clock = bestClock(frequency, rc);

	switch (clock)
	{
	case TC_CMR_TCCLKS_TIMER_CLOCK1:
		_frequency[timer] = (double)VARIANT_MCK / 2.0 / (double)rc;
		break;
	case TC_CMR_TCCLKS_TIMER_CLOCK2:
		_frequency[timer] = (double)VARIANT_MCK / 8.0 / (double)rc;
		break;
	case TC_CMR_TCCLKS_TIMER_CLOCK3:
		_frequency[timer] = (double)VARIANT_MCK / 32.0 / (double)rc;
		break;
	default: // TC_CMR_TCCLKS_TIMER_CLOCK4
		_frequency[timer] = (double)VARIANT_MCK / 128.0 / (double)rc;
		break;
	}

	// Set up the Timer in waveform mode which creates a PWM
	// in UP mode with automatic trigger on RC Compare
	// and sets it up with the determined internal clock as clock input.
	TC_Configure(t.tc, t.channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | clock);
	// Reset counter and fire interrupt when RC value is matched:
	TC_SetRC(t.tc, t.channel, rc);
	// Enable the RC Compare Interrupt...
	t.tc->TC_CHANNEL[t.channel].TC_IER = TC_IER_CPCS;
	// ... and disable all others.
	t.tc->TC_CHANNEL[t.channel].TC_IDR = ~TC_IER_CPCS;

	return *this;
}

DueTimer &DueTimer::setPeriod(unsigned long microseconds)
{
	/*
		Set the period of the timer (in microseconds)
	*/

	// Convert period in microseconds to frequency in Hz
	double frequency = 1000000.0 / microseconds;
	setFrequency(frequency);
	return *this;
}

double DueTimer::getFrequency(void) const
{
	/*
		Get current time frequency
	*/

	return _frequency[timer];
}

long DueTimer::getPeriod(void) const
{
	/*
		Get current time period
	*/

	return 1.0 / getFrequency() * 1000000;
}

/*
	Implementation of the timer callbacks defined in
	arduino-1.5.2/hardware/arduino/sam/system/CMSIS/Device/ATMEL/sam3xa/include/sam3x8e.h
*/
// Fix for compatibility with Servo library
void TC0_Handler(void)
{
	TC_GetStatus(TC0, 0);
	DueTimer::callbacks[0]();
}
void TC1_Handler(void)
{
	TC_GetStatus(TC0, 1);
	DueTimer::callbacks[1]();
}
// Fix for compatibility with Servo library
void TC2_Handler(void)
{
	TC_GetStatus(TC0, 2);
	DueTimer::callbacks[2]();
}
void TC3_Handler(void)
{
	TC_GetStatus(TC1, 0);
	DueTimer::callbacks[3]();
}
void TC4_Handler(void)
{
	TC_GetStatus(TC1, 1);
	DueTimer::callbacks[4]();
}
void TC5_Handler(void)
{
	TC_GetStatus(TC1, 2);
	DueTimer::callbacks[5]();
}
void TC6_Handler(void)
{
	TC_GetStatus(TC2, 0);
	DueTimer::callbacks[6]();
}
void TC7_Handler(void)
{
	TC_GetStatus(TC2, 1);
	DueTimer::callbacks[7]();
}
void TC8_Handler(void)
{
	TC_GetStatus(TC2, 2);
	DueTimer::callbacks[8]();
}
#endif