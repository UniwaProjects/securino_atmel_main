#include "SerialManager.h"

//using serial::SerialManager;

serial::SerialManager::SerialManager()
{
	m_serial_buffer = new CharBuffer(max_buffer_size);
}

//Default baud rate is 9600, since its a moderate
//speed for small amount of data with small error rate.
void serial::SerialManager::init(uint32_t baud_rate)
{
	Serial.begin(baud_rate);
}

//If a serial connection is available it reads the entire Serial Buffer
//searching for a command, copies it into our own serial buffer and then
//return true. Otherwise if there is no command to read it returns false.
//The purpose of this transfer is that serial data are preserved and can be read
//from multiple functions without losing the data upon reading them as it
//happens with the original serial buffer.
bool serial::SerialManager::getCommand()
{
	//If there are no bytes to read, exit
	if (!Serial.available())
	{
		return false;
	}

	//If there is not a command on buffer, exit
	if (!Serial.find("CMD+"))
	{
		return false;
	}

	//Load the command to the buffer
	delay(100); //WITHOUT THIS DELAY EVERYTHING FALLS APART
	m_serial_buffer->clear();
	//Read all the bytes up to /n (/r/n is the default serial
	//end of lines), avoiding copying those special characters
	uint8_t i = 0;
	char c = 0;
	do
	{
		c = (char)Serial.read();
		if (c != '\n' && c != '\r')
		{
			m_serial_buffer->setChar(i, c);
			i++;
		}
	} while (c != '\n');
	return true;
}

//Reads leftover characters and clears the custom serial buffer
//so that the find() of the next commands wont have to search
//through trash or read an earlier response.
void serial::SerialManager::clearSerial()
{
	m_serial_buffer->clear();
	while (Serial.read() != -1)
		;
}

//Sends the state via Serial to the ESP. If the ESP replies with
//an OK response it returns true, otherwise false.
bool serial::SerialManager::sendStatus(const alarm::Status &current_status)
{
	Serial.print(F("CMD+STATUS:"));
	Serial.print(current_status.state);
	Serial.print(F(","));
	Serial.print(current_status.method);
	Serial.print(F(","));
	Serial.println(current_status.sensor);
	return getResponse("RSP+OK", response_timeout_mils);
}

//Searches the buffer for the "STATE" command. If the command is found
//and the values are within the desired range it returns the new state. In any
//different case (couldn't fetch the command, bad values), it returns the
//previous state.
alarm::Status serial::SerialManager::readStatus(const alarm::Status &current_status)
{
	char *command = "STATUS";
	//If the command cannot be found, exit
	if (!m_serial_buffer->find(command))
	{
		return current_status;
	}

	alarm::Status new_status = {alarm::state_disarmed, alarm::method_none, alarm::sensor_none_triggered};
	//Skips the ":" after the command and gets the first number
	//which is state
	uint8_t index = strlen(command) + 1;
	uint8_t state = m_serial_buffer->getInt(index);
	//Skip the ',' and read the next number which is arm method
	index = index + 2;
	uint8_t arm = m_serial_buffer->getInt(index);
	//Skip the ',' and read the last number which is sensor state
	index = index + 2;
	uint8_t sensor = m_serial_buffer->getInt(index);
	//If everything is within limits
	if ((state >= 0 && state <= 2) && (arm >= 0 && arm <= 2) && (sensor >= 0 && sensor <= 3))
	{
		new_status.state = (alarm::arm_state_t)state;
		new_status.method = (alarm::arm_method_t)arm;
		new_status.sensor = (alarm::sensor_state_t)sensor;
		Serial.println(F("RSP+OK"));
		return new_status;
	}
	else
	{
		Serial.println(F("RSP+BAD_VALUE"));
		return current_status;
	}
}

//Waits the desired amount of time and then reads the response returning
//true if found or false otherwise
bool serial::SerialManager::getResponse(char *response, uint16_t wait)
{
	if (wait > 0)
	{
		delay(wait);
	}
	return Serial.find(response);
}