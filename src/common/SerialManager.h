/*
A serial manager whose purpose is to keep the copied buffer object, manipulate
it and reading the status from it, which will be a function shared across Arduino and
ESP8266.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "CharBuffer.h"
#include "alarmtypes.h"

namespace serial
{
	const uint8_t max_buffer_size = 64; //64 is the max size of the arduino's serial buffer
	const uint16_t response_timeout_mils = 500;

	class SerialManager
	{
	public:
		void init(uint32_t baud_rate = 9600);
		bool getCommand();
		void clearSerial();
		bool sendStatus(const alarm::Status &current_status);
		alarm::Status readStatus(const alarm::Status &current_status);

	protected:
		//Methods
		SerialManager();
		bool getResponse(char *response, uint16_t wait);
		//Variables
		CharBuffer *m_serial_buffer;
	};
} // namespace serial