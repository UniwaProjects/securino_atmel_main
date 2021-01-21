#include "SpecializedSerial.h"

serial::SpecializedSerial *serial::SpecializedSerial::m_instance = nullptr;

serial::SpecializedSerial *serial::SpecializedSerial::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SpecializedSerial();
	}
	return m_instance;
}

serial::SpecializedSerial::SpecializedSerial() {}

//Is used to request a wifi network change from the ESP.
//The esp replies with an OK response and then a list of networks
//to choose from. Returns true or false depending on the response.
bool serial::SpecializedSerial::sendNetChange()
{
	Serial.println(F("CMD+CHANGE"));
	if (getResponse("RSP+OK", response_timeout_mils))
	{
		return true;
	}
	return false;
}

//Sends a retry to connect command to ESP.
bool serial::SpecializedSerial::sendRetry()
{
	Serial.println(F("CMD+RETRY"));
	if (getResponse("RSP+OK", response_timeout_mils))
	{
		return true;
	}
	return false;
}

//Requests a hardware reset on the ESP. An OK
//response is expected.
bool serial::SpecializedSerial::sendReset()
{
	Serial.println(F("CMD+RESET"));
	if (getResponse("RSP+OK", response_timeout_mils))
	{
		return true;
	}
	return false;
}

//Sends wifi network credentials such as ssid and a password.
//An ok response is expected and true is returned if received.
bool serial::SpecializedSerial::sendNetCredentials(const char *ssid, const char *pass)
{
	Serial.print(F("CMD+CREDENTIALS:"));
	Serial.print(ssid);
	Serial.print(',');
	Serial.println(pass);
	if (getResponse("RSP+OK", response_timeout_mils))
	{
		return true;
	}
	return false;
}

uint32_t serial::SpecializedSerial::readDeviceId()
{
	char *command = "DEVICE_ID";
	if (!m_serial_buffer->find(command))
	{
		return 0;
	}

	//Skips the ":" after the command and gets the first char array
	uint8_t array_index = 0;
	uint8_t buffer_index = strlen(command) + 1;
	char id_array[12] = {0};
	do
	{
		id_array[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > 11)
		{
			Serial.println(F("RSP+OK"));
			return 0;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != '\0');

	Serial.println(F("RSP+OK"));
	return strtoul(id_array, NULL, 0);
}

//Looks for a received command  in the form of "NET_INFO:SSID,RSSI,LOCALIP"
//and if such a command with parameters within the allowed length is received,
//a new network::Info obJect is returned containing those. Otherwise an empty wifiinfo
//object is returned.
network::Info serial::SpecializedSerial::readNetInfo()
{
	network::Info bad_info = {"INVALID NET NAME", -100, "0.0.0.0"};
	char *command = "INFO";
	if (!m_serial_buffer->find(command))
	{
		return bad_info;
	}

	//Skips the ":" after the command and gets the first char array
	//which is ssid.
	uint8_t array_index = 0;
	uint8_t buffer_index = strlen(command) + 1;
	network::Info new_info = {0, 0, 0};
	do
	{
		new_info.ssid[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > network::max_credential_length)
		{
			//Serial.println(F("RSP+BAD_SSID_LENGTH"));
			Serial.println(F("RSP+OK"));
			return bad_info;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != ',');

	//Skips the ',' character and gets the second
	//char array which is rssi.
	array_index = 0;
	buffer_index++;
	char rssi[5] = {0};
	do
	{
		rssi[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > 5)
		{
			//Serial.println(F("RSP+BAD_RSSI"));
			Serial.println(F("RSP+OK"));
			return bad_info;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != ',');
	//The buffer is then converted to int
	new_info.rssi = atoi(rssi);

	//Skips the ',' character and gets the last
	//char array which is ip.
	array_index = 0;
	buffer_index++;
	do
	{
		new_info.local_ip[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > network::max_ip_length)
		{
			//Serial.println(F("RSP+BAD_IP_LENGTH"));
			Serial.println(F("RSP+OK"));
			return bad_info;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != '\0');

	//Report OK and return the new info object
	Serial.println(F("RSP+OK"));
	return new_info;
}

//Reads the buffer for a network disconnected command and returns
//true if found, false otherwise. Then responds with a waiting
//networks response.
bool serial::SpecializedSerial::readNetworkDisconnected()
{
	char *command = "DISCONNECTED";
	if (m_serial_buffer->find(command))
	{
		Serial.println(F("RSP+OK"));
		return true;
	}
	return false;
}

//Reads the buffer for a network list header, which consists of
//a networks start message, followed by the number of networks to come.
//The returned number is the number of networks or -1 if too many networks
//were received or the command wasn't found.
//The maximum number of networks allowed are 99.
int8_t serial::SpecializedSerial::readNetworkHeader()
{
	char *command = "START_LIST";
	if (!m_serial_buffer->find(command))
	{
		return -1;
	}

	uint8_t array_index = 0;
	uint8_t buffer_index = strlen(command) + 1;
	//A 3 char array, 2 chars for the number and 1 for the terminator.
	char networks[3] = {0};
	do
	{
		networks[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		//If a bigger than a 2 digit number is received
		if (array_index > 2)
		{
			//Serial.println(F("RSP+TOO_MANY_NETWORKS"));
			Serial.println(F("RSP+OK"));
			return -1;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != '\0');

	//Otherwise report OK
	Serial.println(F("RSP+OK"));
	return atoi(networks);
}

//Reads a wifi network from buffer in the form of "NETWORK:SSID,RSSI,ENCRYPTION".
//If the parameters are within accepted lengths, a wifinetwork object filled
//with the information received is returned and an OK response is sent via serial.
//Otherwise an empty object is returned and a response with error information.
network::ScannedNetwork serial::SpecializedSerial::readNetwork()
{
	network::ScannedNetwork bad_network = {"INVALID NET NAME", -100, network::encrytpion_none};
	char *command = "NETWORK";
	if (!m_serial_buffer->find(command))
	{
		return bad_network;
	}
	// Skips the ":" after the command and gets the first
	//char array which is ssid
	uint8_t array_index = 0;
	uint8_t buffer_index = strlen(command) + 1;
	network::ScannedNetwork network = {0, 0, 0};
	do
	{
		network.ssid[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > network::max_credential_length)
		{
			//Serial.println(F("RSP+BAD_SSID_LENGTH"));
			Serial.println(F("RSP+OK"));
			return bad_network;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != ',');
	// Skips the "," character and gets the second
	//char array which is rssi
	array_index = 0;
	buffer_index++;
	char rssi[5] = {0};
	do
	{
		rssi[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > 4)
		{
			//Serial.println(F("RSP+BAD_RSSI"));
			Serial.println(F("RSP+OK"));
			return bad_network;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != ',');
	//Convert the rssi char array to int
	network.rssi = atoi(rssi);
	// Skips the "," character and gets the last
	//char array which is local ip
	array_index = 0;
	buffer_index++;
	network.encryption = (network::encryption_t)m_serial_buffer->getInt(buffer_index);
	//If the next character is not the termination character
	if (m_serial_buffer->getChar(buffer_index + 1) != '\0')
	{
		//Serial.println(F("RSP+BAD_ENCRYPTION"));
		Serial.println(F("RSP+OK"));
		return bad_network;
	}

	//Otherwise report OK
	Serial.println(F("RSP+OK"));
	return network;
}

//Reads the buffer for the networks end command and returns
//true if found or false otherwise.
bool serial::SpecializedSerial::readNetworkEnd()
{
	char *command = "END_LIST";
	if (m_serial_buffer->find(command))
	{
		Serial.println(F("RSP+OK"));
		return true;
	}
	return false;
}