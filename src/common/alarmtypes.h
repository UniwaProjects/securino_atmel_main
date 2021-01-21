#pragma once

namespace alarm
{
	//The following enums construct the final alarm::Status struct, which consists of the
	//alarm state, the arm method and the sensor state.
	typedef enum arm_state_t
	{
		state_disarmed = 0,
		state_armed = 1,
		state_alert = 2
	} arm_state_t;

	typedef enum arm_method_t
	{
		method_none = 0,
		method_arm_stay = 1,
		method_arm_away = 2
	} arm_method_t;

	typedef enum sensor_state_t
	{
		sensor_none_triggered = 0,
		sensor_one_triggered = 1,
		sensor_multiple_triggered = 2,
		sensor_offline = 3
	} sensor_state_t;

	typedef struct
	{
		arm_state_t state;
		arm_method_t method;
		sensor_state_t sensor;
	} Status;
} // namespace alarm