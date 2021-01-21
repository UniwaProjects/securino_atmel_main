#include "SavedData.h"

data::SavedData *data::SavedData::m_instance = nullptr;

data::SavedData *data::SavedData::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new data::SavedData();
	}
	return m_instance;
}

data::SavedData::SavedData() {}

// Initialized the EEPROM the first time the controller boots
void data::SavedData::initializeMemory()
{
	uint8_t value = EEPROM.read(memoryInitAddress);
	if (value == memoryInitValue)
	{
		return;
	}

	savePin("1234");
	saveSessionId(0);
	saveNextSensorId(0);
	saveRegisteredSensorCount(0);

	EEPROM.write(memoryInitAddress, memoryInitValue);
}

// Returns false if a pin larger than the allowed is given.
// Else saves pin in EEPROM addresses 0 to 3 and then returns true.
bool data::SavedData::savePin(const char *pin)
{
	// If the pin exceeds the allowed char number, return
	if (strlen(pin) > data::pin_length + 1)
	{
		return false;
	}
	// Else save pin
	for (uint8_t i = 0; i < data::pin_length; i++)
	{
		EEPROM.write(pin_address + i, pin[i]);
	}

	return true;
}

// Reads the first 4 addresses of the EEPROM and returns
// a string of the pin.
String data::SavedData::readPin()
{
	char pinBuffer[data::pin_length + 1] = {0};
	for (uint8_t i = 0; i < data::pin_length; i++)
	{
		pinBuffer[i] = (char)EEPROM.read(pin_address + i);
	}
	return String(pinBuffer);
}

// Saves the session id as a char array.
void data::SavedData::saveSessionId(uint16_t session_id)
{
	char buffer[session_id_length + 1];
	memset(buffer, 0, session_id_length + 1);
	sprintf(buffer, "%u", session_id);
	for (int i = 0; i < session_id_length; i++)
	{
		EEPROM.write(session_id_address + i, buffer[i]);
	}
}

// Reads the session id from the specified address.
// Returns it as an unsigned integer.
uint16_t data::SavedData::readSessionId()
{
	char buffer[session_id_length + 1] = {0};
	for (int i = 0; i < session_id_length; i++)
	{
		char c = (char)EEPROM.read(session_id_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}

// Saves the sensor id as a char array.
void data::SavedData::saveNextSensorId(uint8_t sensor_id)
{
	char buffer[sensor_id_length + 1];
	memset(buffer, 0, sensor_id_length + 1);
	sprintf(buffer, "%hu", sensor_id);
	for (int i = 0; i < sensor_id_length; i++)
	{
		EEPROM.write(sensor_id_address + i, buffer[i]);
	}
}

// Reads the sensor id from the specified address.
// Returns it as an unsigned short integer.
uint8_t data::SavedData::readNextSensorId()
{
	char buffer[sensor_id_length + 1] = {0};
	for (int i = 0; i < sensor_id_length; i++)
	{
		char c = (char)EEPROM.read(sensor_id_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}

// Saves the sensor id as a char array.
void data::SavedData::saveRegisteredSensorCount(uint8_t sensor_count)
{
	char buffer[sensor_count_length + 1];
	memset(buffer, 0, sensor_count_length + 1);
	sprintf(buffer, "%hu", sensor_count);
	for (int i = 0; i < sensor_count_length; i++)
	{
		EEPROM.write(sensor_count_address + i, buffer[i]);
	}
}

// Reads the sensor id from the specified address.
// Returns it as an unsigned short integer.
uint8_t data::SavedData::readRegisteredSensorCount()
{
	char buffer[sensor_count_length + 1] = {0};
	for (int i = 0; i < sensor_count_length; i++)
	{
		char c = (char)EEPROM.read(sensor_count_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}