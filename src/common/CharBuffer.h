/*
A char buffer which is meant to hold the contents of the serial buffer since
the serial buffer is cleared upon reading and can't be read multiple times.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class CharBuffer
{
public:
	CharBuffer(uint16_t requested_size);
	~CharBuffer();
	char getChar(uint16_t index);
	int8_t getInt(uint16_t index);
	bool setChar(uint16_t index, char character);
	bool find(const char *word);
	String toString();
	void clear();

private:
	//Variables
	uint8_t m_buffer_size;
	char *m_buffer;
};