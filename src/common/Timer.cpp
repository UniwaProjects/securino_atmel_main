#include "Timer.h"

Timer::Timer(uint16_t seconds)
{
	m_timeout_seconds = seconds;
	m_timer = millis();
}

//Sets the timer to current milliseconds using
//a function from Arduino.h, esentially resetting it.
void Timer::reset()
{
	m_timer = millis();
}

//Returns true if the current milliseconds minus the last
//millisecond entry are larger than the timeout(multiplied by 1000
//to convert them to seconds).
bool Timer::timeout()
{
	if (millis() - m_timer > (m_timeout_seconds * 1000))
	{
		return true;
	}
	return false;
}