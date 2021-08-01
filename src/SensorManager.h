/*
Handles the RF24L01+ and RF24 libraries for the management
of the mesh network. Keeps track of all registered sensors and handles all
the communication.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "common/sensortypes.h"
#include "common/alarmtypes.h"

namespace sensors
{
	//Used for the sensors struct, each sensor has a type, an id and a timestamp
	//in order to expire after the designated seconds pass and the sensor didn't ping.
	typedef struct Sensor
	{
		uint8_t sensor_id = 0;
		sensortypes::sensor_type_t type = sensortypes::type_none;
		sensortypes::sensor_state_t state = sensortypes::state_ping;
		uint32_t timestamp = 0;
	} Sensor;
	// Results of sensor pairing
	typedef enum setup_outcome_t
	{
		ok = 1,
		error = 2
	} setup_outcome_t;
	//Radio Variables
	const uint64_t rf24_addresses[2] = {0xABCDABCD71LL, 0x544d52687CLL};
	const uint8_t rf24_channel = 125; //Channel changes the communication frequency
	//Management constants
	const uint16_t connection_timeout = 30000; //Millis for sensor to communicate
	const uint8_t max_sensors = 6;			   //Max number of sensors in the network
	const uint16_t waiting_timeout_secs = 60;
	//I2C constants
	const int i2c_address = 8;

	class SensorManager
	{
	public:
		SensorManager(SensorManager const &) = delete;
		void operator=(SensorManager const &) = delete;
		//Methods
		static SensorManager *getInstance();
		void init(uint8_t ce_pin, uint8_t csn_pin, uint32_t device_id);
		void clearSensorArray();
		void resetSensorStates();
		void newSession();
		bool canAddSensor();
		bool pair();
		bool listen(const alarm::Status &status);
		uint8_t triggeredCount();
		int isOffline();
		int hasLowBattery();
		uint8_t getMagnetCount();
		uint8_t getPirCount();

	private:
		//Methods
		SensorManager();
		sensortypes::SensorAck createAck(const alarm::Status &status);
		bool handleMessage(uint8_t sensor_id, sensortypes::sensor_type_t type, sensortypes::sensor_state_t state);
		bool registerSensor(uint8_t sensor_id, sensortypes::sensor_type_t type);
		void increaseCounterOfType(sensortypes::sensor_type_t type);
		//Variables
		static SensorManager *m_instance;
		sensors::Sensor *m_sensors;
		uint8_t m_pir_counter;
		uint8_t m_magnet_counter;
		uint16_t m_session_id;
		uint32_t m_device_id;
		uint8_t m_next_sensor_id;
		RF24 *m_radio;
	};
} // namespace sensors
