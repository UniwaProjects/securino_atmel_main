#include "SensorManager.h"
#include "SavedData.h"
#include "common/Timer.h"

//#define DEBUG

sensors::SensorManager *sensors::SensorManager::m_instance = nullptr;
data::SavedData *m_data = data::SavedData::getInstance();

sensors::SensorManager *sensors::SensorManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SensorManager();
	}
	return m_instance;
}

//Locks the array memory and initialize it.
sensors::SensorManager::SensorManager()
{
	Wire.begin();
	m_sensors = (Sensor *)malloc(max_sensors * sizeof(Sensor));
	clearSensorArray();
}

//Initialize and configure Radio.
void sensors::SensorManager::init(uint8_t ce_pin, uint8_t csn_pin, uint32_t device_id)
{
	m_device_id = device_id;
	m_session_id = m_data->readSessionId();
	m_next_sensor_id = m_data->readNextSensorId();

	m_radio = new RF24(ce_pin, csn_pin);
	m_radio->begin();
	m_radio->setPALevel(RF24_PA_MIN);
	m_radio->setChannel(rf24_channel);
	m_radio->setAutoAck(true);									 //Ensure autoACK is enabled
	m_radio->enableAckPayload();								 //Allow optional ack payloads
	m_radio->setPayloadSize(sizeof(sensortypes::SensorMessage)); //Here we are sending 1-byte payloads to test the call-response speed
	//Open pipes
	m_radio->openWritingPipe(rf24_addresses[1]);	//Both radios listen on the same pipes by default, and switch when writing
	m_radio->openReadingPipe(1, rf24_addresses[0]); //Open reading pipe
	m_radio->startListening();						// Start listening
}

void sensors::SensorManager::clearSensorArray()
{
	m_magnet_counter = 0;
	m_pir_counter = 0;
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		m_sensors[i].sensor_id = 0;
		m_sensors[i].type = sensortypes::type_none;
		m_sensors[i].state = sensortypes::state_ping;
		m_sensors[i].timestamp = 0;
	}
}

//Resets the counters, ID and clears the array.
void sensors::SensorManager::newSession()
{
	clearSensorArray();
	m_data->saveRegisteredSensorCount(0);
	m_session_id++;
	m_data->saveSessionId(m_session_id);
#ifdef DEBUG
	Serial.println("Ids after newSession: " + String(m_device_id) + ", " + String(m_session_id) + ", " + String(m_next_sensor_id));
#endif
}

// Installs the connected sensor using I2C, returns false if a connection was
// not established or memory could not be allocated for the sensor.
bool sensors::SensorManager::pair()
{
	Timer waiting_timer(waiting_timeout_secs);
	while (1)
	{
		if (waiting_timer.timeout())
		{
			return false;
		}

		//Request the sensor type. 0 is returned if the request cannot reach the slave.
		if (Wire.requestFrom(i2c_address, sizeof(uint8_t)) == 0)
		{
			continue;
		}

		//Read requested byte.
		int sensor_type_id = Wire.read();

		//Returns -1 if nothing was read.
		if (sensor_type_id <= 0)
		{
			continue;
		}

		sensortypes::sensor_type_t sensor_type = (sensortypes::sensor_type_t)sensor_type_id;
		//The message for the sensor.
		String message = String(m_device_id) + "," + String(m_session_id) + "," + String(m_next_sensor_id);

#ifdef DEBUG
		Serial.print("Wire received sensor type: ");
		Serial.println(sensor_type);
		Serial.println(message);
		Serial.println(m_device_id);
#endif
		//Transmit message.
		Wire.beginTransmission(i2c_address);
		Wire.write(message.c_str());
		Wire.endTransmission();

		//2 is bad, 1 is good
		int sensor_response = -1;
		while (1)
		{
			//Request the response. 0 is returned if the request cannot reach the slave.
			if (Wire.requestFrom(i2c_address, sizeof(uint8_t)) > 0)
			{
				//Read requested byte.
				sensor_response = Wire.read();
				//Returns -1 if nothing was read.
				if (sensor_response > 0)
				{
					break;
				}
			}
		}

#ifdef DEBUG
		Serial.print("Wire received response: ");
		Serial.println(sensor_response);
#endif

		if (sensor_response == setup_outcome_t::error)
		{
			return false;
		}

		//Add sesnor to array if possible.
		if (!registerSensor(m_next_sensor_id, sensor_type))
		{
			continue;
		}

		//Increment next sensor id.
		m_next_sensor_id++;

		//0 is reserved for no ID
		if (m_next_sensor_id == 0)
		{
			m_next_sensor_id = 1;
		}
		m_data->saveNextSensorId(m_next_sensor_id);
#ifdef DEBUG
		Serial.println("Ids after Install: " + String(m_device_id) + ", " + String(m_session_id) + ", " + String(m_next_sensor_id));
#endif

		return true;
	}
}

// Create an ack with the given status and device info
sensortypes::SensorAck sensors::SensorManager::createAck(const alarm::Status &status)
{
	sensortypes::SensorAck sensorAck;
	sensorAck.parent_device_id = m_device_id;
	sensorAck.session_id = m_session_id;

	//Response is 1 for armed 0 for not armed or on alert.
	if (status.state == alarm::state_armed)
	{
		//If the method is arm stay, pir sensors should not work, so send a disarmed response.
		if (status.method == alarm::method_arm_stay)
		{
			sensorAck.sensors_to_arm = sensortypes::sensor_type_t::type_magnet;
		}
		else
		{
			sensorAck.sensors_to_arm = sensortypes::sensor_type_t::type_pir;
		}
	}
	else
	{
		sensorAck.sensors_to_arm = sensortypes::sensor_type_t::type_none;
	}
	return sensorAck;
}

//Listens for sensor messages.
bool sensors::SensorManager::listen(const alarm::Status &status)
{
	uint8_t pipe_number;

	//If no incomming messages, exit.
	if (!m_radio->available(&pipe_number))
	{
		return false;
	}

	// Prepare the ack
	sensortypes::SensorAck ack = createAck(status);
	m_radio->writeAckPayload(pipe_number, &ack, sizeof(sensortypes::SensorAck));

#ifdef DEBUG
	Serial.println("Ack: " + String(ack.parent_device_id) + ", " + String(ack.session_id) + ", " + String(ack.sensors_to_arm));
#endif

	//Read message.
	sensortypes::SensorMessage message;
	m_radio->read(&message, sizeof(message));

#ifdef DEBUG
	if (message.sensor_id != 0)
	{
		Serial.println("Received: " + String(message.parent_device_id) + ", " + String(message.session_id) + ", " + String(message.sensor_id) + ", " + String(message.state));
	}
#endif

	// If the session and device id match, update the sensor info.
	if (message.session_id == m_session_id && message.parent_device_id == m_device_id)
	{
		handleMessage(message.sensor_id, message.type, message.state);
	}
	else
	{
#ifdef DEBUG
		Serial.println("Rejected: " + String(message.session_id == m_session_id ? "Valid" : "Invalid") + " session ID, " + String(message.parent_device_id == m_device_id ? "Valid" : "Invalid") + " device ID.");
#endif
	}
	return true;
}

//Checks the sensor array for triggered sensors and returns the number of triggered sensors found.
uint8_t sensors::SensorManager::triggeredCount()
{
	uint8_t triggered_count = 0;
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		if ((m_sensors + i)->state == sensortypes::state_triggered)
		{
			triggered_count++;
		}
	}
	return triggered_count;
}

// Reset all sensor states. Used when arming or disarming to start with a fresh array.
void sensors::SensorManager::resetSensorStates()
{
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		(m_sensors + i)->state = sensortypes::state_ping;
	}
}

// Checks for offline sensors and returns the first offline sensor id found
int sensors::SensorManager::isOffline()
{
#ifdef DEBUG
	Serial.println("-----------\nAll sensors\n-----------");
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		Serial.println(String(m_sensors[i].sensor_id) + ", " + String(m_sensors[i].type) + ", " + String(m_sensors[i].state) + ", " + String(m_sensors[i].timestamp));
	}
#endif

	uint32_t current_time = millis();
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		// For a registred sensor
		if (m_sensors[i].sensor_id > 0)
		{
			// If the sensor is timed out
			uint32_t remaining_time = current_time - m_sensors[i].timestamp;
			if (remaining_time >= connection_timeout)
			{
#ifdef DEBUG
				Serial.println("Offline Check: " + String(m_sensors[i].sensor_id) + ", " + String(m_sensors[i].state) + ", " + String(m_sensors[i].timestamp));
#endif
				// Return the first sensor id
				return i;
			}
		}
	}
	return -1;
}

// Checks for offline sensors and returns the first low on battery sensor id found
int sensors::SensorManager::hasLowBattery()
{
	uint32_t current_time = millis();
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		// For a registred sensor
		if (m_sensors[i].sensor_id > 0)
		{
			// If the sensor has low battery and is not offline
			uint32_t remaining_time = current_time - m_sensors[i].timestamp;
			if (m_sensors[i].state == sensortypes::state_battery_low && remaining_time < connection_timeout)
			{
#ifdef DEBUG
				Serial.println("Low Battery Check: " + String(m_sensors[i].sensor_id) + ", " + String(m_sensors[i].state) + ", " + String(m_sensors[i].timestamp));
#endif
				// Return the first sensor id
				return i;
			}
		}
	}
	return -1;
}

//Returns the magnet sensor count.
uint8_t sensors::SensorManager::getMagnetCount()
{
	return m_magnet_counter;
}

//Returns the pir sensor count.
uint8_t sensors::SensorManager::getPirCount()
{
	return m_pir_counter;
}

//Updates the sensor info for the given sensor id. Also renews the timestamp.
bool sensors::SensorManager::handleMessage(uint8_t sensor_id, sensortypes::sensor_type_t type, sensortypes::sensor_state_t state)
{
	//For each sensor in the pointer array ..
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		//.. if an id match is found ..
		if (m_sensors[i].sensor_id == sensor_id)
		{
			//.. update the state and the timestamp.
			m_sensors[i].state = state;
			m_sensors[i].timestamp = millis();
			return true;
		}
	}
	//If the sensor is not found yet, add it to an empty reserved space.
	//It has probably reconnected after a main device reboot
	for (uint8_t i = 0; i < max_sensors; i++)
	{
		//If this is the first time the sensor communicates after a reboot of this device
		if (m_sensors[i].type == sensortypes::type_none)
		{
			m_sensors[i].sensor_id = sensor_id;
			m_sensors[i].type = type;
			m_sensors[i].state = state;
			m_sensors[i].timestamp = millis();
			increaseCounterOfType(type);
			return true;
		}
	}

	return false;
}

//Returns true for an array that has the space to add sensors
bool sensors::SensorManager::canAddSensor()
{
	return m_data->readRegisteredSensorCount() < max_sensors;
}

//Registers a new sensor by addings it into the sensor array, if the array is not full.
bool sensors::SensorManager::registerSensor(uint8_t sensor_id, sensortypes::sensor_type_t type)
{
	//If the pointer array isn't full ..
	if ((m_magnet_counter + m_pir_counter) >= max_sensors)
	{
		return false;
	}
	//.. find an empty spot.
	int8_t empty_index = -1;
	for (int8_t i = 0; i < max_sensors; i++)
	{
		if (m_sensors[i].sensor_id == 0)
		{
			empty_index = i;
			break;
		}
	}

	//If an empty index has been found ..
	if (empty_index >= 0)
	{
		//.. add the sensor there.
		m_sensors[empty_index].sensor_id = sensor_id;
		m_sensors[empty_index].type = type;
		m_sensors[empty_index].timestamp = millis();

		//Increase the counter for that type of sensor.
		increaseCounterOfType(type);
		m_data->saveRegisteredSensorCount(empty_index + 1);
		return true;
	}
	return false;
}

//Increases the counter of the given type.
void sensors::SensorManager::increaseCounterOfType(sensortypes::sensor_type_t type)
{
	switch (type)
	{
	case sensortypes::type_magnet:
		m_magnet_counter++;
		break;
	case sensortypes::type_pir:
		m_pir_counter++;
		break;
	}
}