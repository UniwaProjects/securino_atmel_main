/*
A collection of user interface strings saved in Flash memory. The strings are read
from the flash memory using the inline text::FString function. This way aside from reducing
ram usage, it is made easy to translate the interface.
*/
#pragma once

namespace texts
{
	const char version[] PROGMEM = "Securino v1.0";
	const char armed[] PROGMEM = "Armed";
	const char magnet[] PROGMEM = "Magnet: ";
	const char pir[] PROGMEM = "|Pir: ";
	const char disarmed[] PROGMEM = "Disarmed";
	const char wifi[] PROGMEM = "WiFi";
	const char menu[] PROGMEM = "A: Menu";
	const char pin[] PROGMEM = "D: PIN";
	const char state_change[] PROGMEM = "Changing State..";
	const char alert_triggered[] PROGMEM = "Alert Triggered";
	const char menu_wifi_info[] PROGMEM = "WiFi Information";
	const char menu_change_wifi[] PROGMEM = "Change  WiFi";
	const char menu_change_pin[] PROGMEM = "Change PIN";
	const char setup_sensors[] PROGMEM = "Setup Sensors";
	const char setup_sensors_waiting[] PROGMEM = "Waiting . .";
	const char setup_sensor_array_full_1[] PROGMEM = "Max Sensors";
	const char setup_sensor_array_full_2[] PROGMEM = "Reached";
	const char setup_sensor_exists_1[] PROGMEM = "Sensor Already";
	const char setup_sensor_exists_2[] PROGMEM = "Registered";
	const char setup_sensors_menu_line_1[] PROGMEM = "A: Clear All";
	const char setup_sensors_menu_line_2[] PROGMEM = "B: Add Sensor";
	const char setup_sensors_add_line_1[] PROGMEM = "A: Add another";
	const char setup_sensors_add_line_2[] PROGMEM = "B: Finish";

	const char sensors_offline[] PROGMEM = "Sensors Offline";
	const char sensor_x[] PROGMEM = "Sensor X";
	const char sensor_offline[] PROGMEM = "is Offline";
	const char sensor_low_battery[] PROGMEM = "Low Battery";
	const char battery_low[] PROGMEM = "Battery Lo on ";
	const char menu_load_defaults[] PROGMEM = "Factory Defaults";
	const char menu_reset[] PROGMEM = "Reset";
	const char menu_keys[] PROGMEM = "B  C: Enter  A";
	const char menu_keys_last[] PROGMEM = "B  C: Enter";
	const char proceed_line_1[] PROGMEM = "Proceed?";
	const char proceed_line_2[] PROGMEM = "A: Yes B: No";
	const char defaults_loaded[] PROGMEM = "Defaults  Loaded";
	const char enter_pin[] PROGMEM = "PIN: ";
	const char new_pin[] PROGMEM = "New PIN: ";
	const char pin_changed[] PROGMEM = "PIN  Changed";
	const char correct[] PROGMEM = "Correct";
	const char incorrect[] PROGMEM = "Incorrect";
	const char timed_out[] PROGMEM = "Timed Out";
	const char arm_select_line_1[] PROGMEM = "A: Arm Away";
	const char arm_select_line_2[] PROGMEM = "B: Arm Stay";
	const char arm_delay_line_1[] PROGMEM = " Seconds Until";
	const char arm_delay_line_2[] PROGMEM = "System is Armed";
	const char wifi_select_line_1[] PROGMEM = "A: Select a WiFi";
	const char wifi_select_line_2[] PROGMEM = "B: Retry";
	const char ssid[] PROGMEM = "SSID";
	const char wifi_connecting[] PROGMEM = "Connecting . . .";
	const char wifi_connected[] PROGMEM = "Connected!";
	const char wifi_disconnect[] PROGMEM = "Connect Failed";
	const char wifi_connect_keys[] PROGMEM = "B C: Connect A";
	const char wifi_connect_keys_single[] PROGMEM = "C: Connect";
	const char wifi_connect_keys_first[] PROGMEM = "   C: Connect A";
	const char wifi_connect_keys_last[] PROGMEM = "B C: Connect";
	const char no_networks_line_1[] PROGMEM = "No WiFi Networks";
	const char no_networks_line_2[] PROGMEM = "Available";
	const char encryption[] PROGMEM = "Encryption: ";
	const char encr_none[] PROGMEM = "NONE";
	const char encr_wep[] PROGMEM = "WEP";
	const char encr_wpa[] PROGMEM = "WPA";
	const char encr_wpa2[] PROGMEM = "WPA2";
	const char encr_auto[] PROGMEM = "AUTO";
	const char character_limit[] PROGMEM = "Up to 16 chrctrs";
	const char local_ip[] PROGMEM = "Local IP";
	const char wifi_pass_keys[] PROGMEM = "ABC #:Acc *:Del";
	const char wifi_scanning[] PROGMEM = " Scanning . . .";
	const char wifi_rescan[] PROGMEM = "D: Rescan";
	const char pin_keys[] PROGMEM = "Accept:# Del:*";

	inline __FlashStringHelper *getFlashString(const char *texts)
	{
		return (__FlashStringHelper *)(texts);
	}
} // namespace texts