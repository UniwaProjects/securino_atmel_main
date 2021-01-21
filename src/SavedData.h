/*
Handles the communication with the EEPROM and the addresses that
the information is stored.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <EEPROM.h>

namespace data
{
	// Address of the memory init cookie
	const uint16_t memoryInitAddress = 512;
	const uint8_t memoryInitValue = 128;
	//Addresses of stored information and length of said information.
	const uint8_t pin_address = 0;
	const uint8_t pin_length = 4;
	const uint8_t session_id_address = pin_address + pin_length;
	const uint8_t session_id_length = 5;
	const uint8_t sensor_id_address = session_id_address + session_id_length;
	const uint8_t sensor_id_length = 3;
	const uint8_t sensor_count_address = sensor_id_address + sensor_id_length;
	const uint8_t sensor_count_length = 1;

	class SavedData
	{
	public:
		SavedData(SavedData const &) = delete;
		void operator=(SavedData const &) = delete;
		//Methods
		static SavedData *getInstance();
		void initializeMemory();
		bool savePin(const char *pin);
		String readPin();
		void saveSessionId(uint16_t session_id);
		uint16_t readSessionId();
		void saveNextSensorId(uint8_t sensor_id);
		uint8_t readNextSensorId();
		void saveRegisteredSensorCount(uint8_t sensor_count);
		uint8_t readRegisteredSensorCount();

	private:
		//Methods
		SavedData();
		//Variables
		static SavedData *m_instance;
	};
} // namespace data