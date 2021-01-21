/*
A simple countdown timer meant to return true with the timeout function if 
it is not reset in bewtween.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class Timer
{
public:
	Timer(uint16_t seconds);
	void reset();
	bool timeout();

private:
	//Variables
	uint16_t m_timeout_seconds;
	uint32_t m_timer;
};