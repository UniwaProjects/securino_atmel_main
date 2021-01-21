#pragma once

namespace sensortypes
{
	//Represents the type of the sensor (magnet or PIR)
	typedef enum sensor_type_t
	{
		type_none = 0,
		type_magnet = 1,
		type_pir = 2
	} sensor_t;

	//States of a sensor.
	typedef enum sensor_state_t
	{
		state_ping = 0,
		state_triggered = 1,
		state_battery_low = 2
	} sensor_state_t;

	//Wrapper for received sensor messages.
	typedef struct SensorMessage
	{
		uint32_t parent_device_id = 0;	   //Parent is this device, up to 4billion.
		uint16_t session_id = 0;		   //Session that its id was given, up to 128k.
		uint8_t sensor_id = 0;			   //Will only go up to 6, which is the max sensors.
		sensor_type_t type = type_none;	   //Type of the sensor.
		sensor_state_t state = state_ping; //The state of the sensor.
	} SensorMessage;
} // namespace sensortypes