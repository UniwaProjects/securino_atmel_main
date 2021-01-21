#include "DisplayManager.h"
#include "KeyManager.h"
#include "SavedData.h"
#include "SensorManager.h"
#include "SoundManager.h"
#include "SpecializedSerial.h"
#include "common/Timer.h"
#include "common/alarmtypes.h"
#include "common/networktypes.h"
#include "texts.h"

//#define DEBUG

typedef enum user_input_t
{
	input_correct = 0,
	input_incorrect = 1,
	input_timeout = 2
} user_input_t;

#pragma region Constants
// Pin related constants
const char default_pin[data::pin_length + 1] = "1234";
const uint8_t pin_timeout_secs = 10;	   // Timeout of pin entry
const uint8_t selection_timeout_secs = 10; // While choosing sensors to be activated
const uint8_t arm_delay_secs = 30;		   // Time until all sensors are activated
// Menu related constants
const uint8_t menu_timeout_secs = 5; // If no button is pressed while in a menu, exit after timeout
const uint8_t menu_tabs = 5;		 // Menu tabs number
const uint8_t key_timeout_secs = 1;	 // Wifi password letter rotation timeout
// Timer constants
const uint8_t alert_delay_secs = 10;
const uint8_t sensor_check_secs = 10;
// Arduino pins
const uint8_t buzzer_pin = 8;	 // Buzzer digital pin
const uint8_t rf24_ce_pin = 9;	 // Pin 9 of Arduino, used for control of RF24.
const uint8_t rf24_csn_pin = 10; // Pin 10 of Arduino, can only be output if using SPI. Chip
#pragma endregion

#pragma region Global Class Object Intances
sensors::SensorManager *g_sensors = sensors::SensorManager::getInstance();
data::SavedData *g_data = data::SavedData::getInstance();
keypad::KeyManager *g_key = keypad::KeyManager::getInstance();
sound::SoundManager *g_sound = sound::SoundManager::getInstance();
display::DisplayManager *g_display = display::DisplayManager::getInstance();
serial::SpecializedSerial *g_serial = serial::SpecializedSerial::getInstance();
#pragma endregion

#pragma region Global Variables
// Pin variables
char g_pin[data::pin_length + 1] = {0};
// State related variables
alarm::Status g_status = {alarm::state_disarmed,
						  alarm::method_none,
						  alarm::sensor_none_triggered};
network::Info g_network_info = {0, 0, 0};
// Timers
Timer g_input_timer = Timer(alert_delay_secs);	 // Timer until alert is triggered between sensor
												 // input and pin entry
Timer g_sensor_timer = Timer(sensor_check_secs); // Timer to check for deactivated sensors
#pragma endregion

#pragma region Forward Declerations
uint32_t getDeviceId();
void disableAlarm();
void displayStatus(bool light_up);
void keypadListener();
void serialListener();
void sensorHealthChecker();
void sensorStateListener();
void sensorSetup();
bool choiceDialog(uint16_t timeout);
// State change related functions
String inputPin(bool hidden);
user_input_t getPinInputOutcome();
void changeState();
// Main menu functions
bool changeNetwork();
bool changePin(const char *new_pin);
void (*resetFunc)(void) = 0;
void mainMenu();
// Wifi related functions
uint16_t getFreeRam();
bool isNetworkConnected();
String insertNetworkPassword();
bool connectNewNetwork();
void onBootConnectNetwork();
bool reconnectNetwork();
#pragma endregion

#pragma region Setup and Helper Functions
void setup()
{
	// Initialize the sound manager
	g_sound->init(buzzer_pin);

	// Initialize the display manager and show boot screen
	g_display->init();
	g_display->showAlertCenter(texts::version);
	delay(display::standard_delay);

	// Initialize communication with the ESP
	g_serial->init();

	// Initialize the EEPROM memory
	g_data->initializeMemory();

#ifdef DEBUG
	Serial.print("PIN: ");
	Serial.println(String(g_data->readPin()));
	Serial.print("SESSION ID: ");
	Serial.println(String(g_data->readSessionId()));
	Serial.print("NEXT SENSOR ID: ");
	Serial.println(String(g_data->readNextSensorId()));
	Serial.print("SENSOR COUNT: ");
	Serial.println(String(g_data->readRegisteredSensorCount()));
#endif

	uint32_t device_id = getDeviceId();
	// Get wifi info or connect to wifi, will only continue after an
	// establised wifi connection
	onBootConnectNetwork();

	// Read EEPROM saved pin
	strncpy(g_pin, g_data->readPin().c_str(), data::pin_length + 1);

	// Initialize radio communications
	g_sensors->init(rf24_ce_pin, rf24_csn_pin, device_id);

	// Display the status screen
	displayStatus(true);
}

/*
 * Gets the device id from the ESP serial command.
 */
uint32_t getDeviceId()
{
	while (1)
	{
		// If command is not found
		if (g_serial->getCommand())
		{
			uint32_t device_id = g_serial->readDeviceId();

			// If the device_id is zero, check the next command
			if (device_id == 0)
			{
				continue;
			}

			g_serial->clearSerial();
			return device_id;
		}
	}
}

/*
 * Device has to connect o wifi to continue. Non blocking as
 * "reconnectNetwork" function handles user input.
 */
void onBootConnectNetwork()
{
	g_display->showAlertCenter(texts::wifi_connecting);
	delay(display::standard_delay);
	bool connected = isNetworkConnected();
	while (!connected)
	{
		connected = reconnectNetwork();
	}
}
#pragma endregion

#pragma region Loop and Helper Functions
void loop()
{
	// First check for online status changes
	serialListener();
	// Then check for alert
	if (g_status.state == alarm::state_alert)
	{
		// A state were the alarm beeps, the user is notified of
		// the break in and a pin or a serial command
		// is expected for the state to change.
		g_serial->sendStatus(g_status);
		displayStatus(true);
		while (g_status.state == alarm::state_alert)
		{
			g_sound->alarm();
			keypadListener();
			serialListener();
		}
		g_sound->stopAlarm();
	}
	// Check sensors' health
	sensorHealthChecker();
	// Check for sensor messages
	sensorStateListener();
	// Listen for a keypad presses
	keypadListener();
	// Turn off display after timeout if no input
	g_display->turnOffBacklight();
}

/*
 * Listens for serial communication activity,  which will be wifi
 * disconnections and state changes.
 */
void serialListener()
{
	// If no command is found, clear the serial and return
	if (!g_serial->getCommand())
	{
		g_serial->clearSerial();
		return;
	}

	// If the network is not connected
	if (g_serial->readNetworkDisconnected())
	{
		// Notify the user when on offline mode, otherwise
		// automatic silent reconnect will take place on the ESP side
		if (g_status.state == alarm::state_disarmed)
		{
			g_serial->clearSerial();
			g_sound->failureTone();
			while (!reconnectNetwork())
				;
			displayStatus(true);
		}
	}
	else
	{
		// If a new status was received
		alarm::Status new_status = g_serial->readStatus(g_status);
		if ((new_status.state != g_status.state) ||
			(new_status.sensor != g_status.sensor))
		{
			g_sound->successTone();
			g_display->showAlertCenter(texts::state_change);
			if (new_status.state == alarm::state_disarmed)
			{
				disableAlarm();
			}
			else
			{
				g_status = new_status;
			}
			displayStatus(true);
		}
	}
	g_serial->clearSerial();
}

/*
 * Checks every x seconds for offline sensors or sensors with low battery.
 * Notifies user according to system's state.
 */
void sensorHealthChecker()
{
	// If the timer is up, check the sensors' well being
	if (!g_sensor_timer.timeout())
	{
		return;
	}

	// Reset timer
	g_sensor_timer.reset();

	//Check for low battery sensors
	int low_battery_sensor_index = g_sensors->hasLowBattery();

	// Check for offline sensors
	int offline_sensor_index = g_sensors->isOffline();

	// Handle according to the system's state
	switch (g_status.state)
	{
	case alarm::state_armed:
		if (offline_sensor_index >= 0)
		{
			// If a sensor is offline, instant alert by returning.
			g_status.sensor = alarm::sensor_offline;
			g_status.state = alarm::state_alert;
			return;
		}
		break;
	case alarm::state_disarmed:
		if (offline_sensor_index >= 0)
		{
			g_sound->failureTone();
			g_display->showSensorNotification(texts::sensor_offline, offline_sensor_index);
			g_display->resetBacklightTimer();
			delay(display::extended_delay);
		}
		else if (low_battery_sensor_index >= 0)
		{
			g_display->showSensorNotification(texts::sensor_low_battery, low_battery_sensor_index);
			g_display->resetBacklightTimer();
			delay(display::extended_delay);
		}
		break;
	}

	// Show status screen
	displayStatus(false);
}

/*
 * Listens for sensor messages and checks for expired sensors.
 * Τhe info of that object is returned and if its a new sensor
 * the screen gets refreshed.
 * If the sensor is triggered, the state changes accordingly.
 */
void sensorStateListener()
{
	// Listen for sensor messages
	g_sensors->listen(g_status);

	// Check for sensor state
	if (g_status.state == alarm::state_armed)
	{
		// Check for triggered sensors, triggered messages will come only if the
		// alarm is armed.
		uint8_t triggered_count = g_sensors->triggeredCount();
		switch (triggered_count)
		{
		case 0:
			// If no sensor is triggered, the timer is reset continiously.
			g_input_timer.reset();
			break;
		case 1:
			g_status.sensor = alarm::sensor_one_triggered;
			break;
		default:
			// A number other than 0 or 1 indicates multiple triggered sensors.
			g_status.sensor = alarm::sensor_multiple_triggered;
			break;
		}

		// If this counter reaches 0, then the alarm was not disarmed within time.
		// This timer must be less than 24, which is the next time the sensor will
		// communicate in a ping state, essentially removing the triggered state.
		if (g_input_timer.timeout())
		{
			g_status.state = alarm::state_alert;
		}
	}
}

/*
 * Listens for a key press, and if its the D key, the pin entry routine to
 * change the state is called. If the system is disabled and the menu button
 * is pressed, the menu routine is called. In any other case including the above
 * the lcd is lit and the lcd timer is reset.
 */
void keypadListener()
{
	// Get next key
	g_key->getNew();
	// Return if no key is pressed
	if (g_key->getCurrent() == keypad::key_none)
	{
		return;
	}
	// Light up the display if a key is pressed
	g_display->resetBacklightTimer();
	// Enter pin for enter pin key
	if (g_key->enterPinPressed())
	{
		g_sound->menuKeyTone();
		changeState();
	}
	// Else enter menu if not armed
	else if (g_key->menuPressed())
	{
		if (g_status.state == alarm::state_disarmed)
		{
			g_sound->menuKeyTone();
			mainMenu();
		}
	}
}
#pragma endregion

/*
 * Disables the alarm by reinitializing all the arm related
 * variables.
 */
void disableAlarm()
{
	g_status.state = alarm::state_disarmed;
	g_status.method = alarm::method_none;
	g_status.sensor = alarm::sensor_none_triggered;
}

/*
 * Displays the current state, with or without the pir sensors
 * depending on the arm method
 */
void displayStatus(bool light_up)
{
	uint8_t state = (uint8_t)g_status.state;
	uint32_t rssi = g_network_info.rssi;
	uint8_t magnet_count = g_sensors->getMagnetCount();
	uint8_t pir_count = g_sensors->getPirCount();
	if (g_status.method == alarm::method_arm_away)
	{
		g_display->showStatus(state, rssi, magnet_count, pir_count);
	}
	else
	{
		g_display->showStatus(state, rssi, magnet_count);
	}
	if (light_up)
	{
		g_display->resetBacklightTimer();
	}
}

/*
 * Awaits input for timeout time, after which false is returned. True is
 * returned for option A and false for option B.
 */
bool choiceDialog(uint16_t timeout)
{
	Timer timer = Timer(timeout);
	while (1)
	{
		// Timeout
		if (timer.timeout())
		{
			g_sound->menuKeyTone();
			return false;
		}
		// Get new key
		g_key->getNew();
		// Option A
		if (g_key->aPressed())
		{
			g_sound->menuKeyTone();
			return true;
		}
		// Option B
		if (g_key->bPressed())
		{
			g_sound->menuKeyTone();
			return false;
		}
	}
}

/*
 * Gets a pin input from the keyboard. Can timeout and return an empty string.
 */
String inputPin(bool pin_hidden)
{
	char input_buffer[data::pin_length + 1] = {0};
	uint8_t index = 0;
	Timer timer = Timer(pin_timeout_secs);
	while (1)
	{
		// Ifp in is hidden stars are displayed instead
		if (pin_hidden)
		{
			g_display->showEnterPin(input_buffer);
		}
		else
		{
			g_display->showEnterNewPin(input_buffer);
		}
		// Try to get a number, non blocking can timeout
		do
		{
			// On timeout return an empty pin, which will be incorrect
			if (timer.timeout())
			{
				return "";
			}
			g_key->getNew();
		} while (!g_key->numberPressed() && !g_key->backspacePressed() &&
				 !g_key->acceptPressed());
		// Sound of click
		g_sound->pinKeyTone();
		// If the accept key is pressed and the pin length is complete, exit
		if (g_key->acceptPressed() && (index == data::pin_length))
		{
			break;
		}
		// Get a number or backspace 1 character
		if (g_key->numberPressed())
		{
			if (index < data::pin_length)
			{
				char input = g_key->getCurrent();
				input_buffer[index] = input;
				index++;
			}
		}
		else if (g_key->backspacePressed())
		{
			if (index > 0)
			{
				index--;
			}
			input_buffer[index] = '\0';
		}
	}
	return String(input_buffer);
}

/*
 * Gets the pin using the inputPin function and returns either correct,
 * incorrect or timeout, depending on user input or lack of input.
 */
user_input_t getPinInputOutcome()
{
	String pin = inputPin(true);
	// If the pin is less than the pin length
	if (pin.length() < data::pin_length)
	{
		return input_timeout;
	}
	// If the pin matches the saved pin
	if (strncmp(pin.c_str(), g_pin, data::pin_length + 1) == 0)
	{
		return input_correct;
	}
	return input_incorrect;
}

/*
 * Depending on the user input and the current alarm state, the state is changed
 * or the tries reduced if the input was wrong while the system is armed.
 */
void changeState()
{
	// Get pin from user and check the result
	user_input_t input_result = getPinInputOutcome();
	// For a CORRECT pin
	if (input_result == input_correct)
	{
		// Play success sound and show the success message
		g_sound->successTone();
		g_display->showAlertCenter(texts::correct);
		delay(display::standard_delay);
		// If at this momement the alarm is disarmed
		if (g_status.state == alarm::state_disarmed)
		{
			// For offline sensors, don't arm
			if (g_sensors->isOffline() > 0)
			{
				g_display->showAlertCenter(texts::sensors_offline);
				delay(display::standard_delay);
				return;
			}
			// Otherwise change the state and arm
			g_status.state = alarm::state_armed;
			g_display->showAlertStart(texts::arm_select_line_1, texts::arm_select_line_2);
			// Enter  preffered arm method, stay or away
			bool is_arm_away = choiceDialog(selection_timeout_secs);
			if (is_arm_away)
			{
				g_status.method = alarm::method_arm_away;
				for (uint8_t i = arm_delay_secs; i > 0; i--)
				{
					g_sound->menuKeyTone();
					g_display->showArmDelay(i);
					delay(1000);
				}
			}
			else
			{
				g_status.method = alarm::method_arm_stay;
			}
			// Reset the sensor timer
			g_sensor_timer.reset();
		}
		else
		{
			// Else the alarm was armed, so disable it
			disableAlarm();
		}
		// Show and send the status change
		g_display->showAlertCenter(texts::state_change);
		g_serial->sendStatus(g_status);
	}
	else
	{
		// Else for timeout or incorrect pin
		g_sound->failureTone();
		g_display->showAlertCenter(texts::incorrect);
		delay(display::standard_delay);
		if (g_status.state == alarm::state_armed)
		{
			g_status.state = alarm::state_alert;
		}
	}
	// Dsiplay the new status
	displayStatus(true);
}

/*
 * Sends a change net command to the ESP, resets wifi info and starts the
 * connect to new net routine.
 */
bool changeNetwork()
{
	// Return if failed to communicate with ESP
	if (!g_serial->sendNetChange())
	{
		return false;
	}
	g_network_info = {0, 0, 0};
	// Wont continue until a connection has been established
	while (!connectNewNetwork())
		;
	return true;
}

/*
 * Returns true if the pin is changed successfully or false otherwise.
 * For the pin to be changed first the correct current pin must be entered
 * and if the new pin that was given is of valid length, the function saves
 * the pin and returns true. In any other case it returns false.
 */
bool changePin(const char *new_pin)
{
	bool result = false;
	user_input_t input = getPinInputOutcome();
	switch (input)
	{
	case input_correct:
		g_sound->successTone();
		g_display->showAlertCenter(texts::correct);
		delay(display::standard_delay);
		// If the new pin is short
		if (*(new_pin + (data::pin_length - 1)) == 0)
		{
			break;
		}
		strncpy(g_pin, new_pin, data::pin_length + 1);
		g_data->savePin(g_pin);
		g_sound->successTone();
		g_display->showAlertCenter(texts::pin_changed);
		delay(display::standard_delay);
		result = true;
		break;

	case input_incorrect:
		g_sound->failureTone();
		g_display->showAlertCenter(texts::incorrect);
		delay(display::standard_delay);
		break;

	case input_timeout:
		g_sound->failureTone();
		g_display->showAlertCenter(texts::timed_out);
		delay(display::standard_delay);
		break;
	}
	return result;
}

void sensorSetup()
{
	// Show dialog and get choice
	g_display->showAlertStart(texts::setup_sensors_menu_line_1, texts::setup_sensors_menu_line_2);
	bool is_clear_sensors = choiceDialog(menu_timeout_secs);

	// Clear sensors
	if (is_clear_sensors)
	{
		g_display->showAlertCenter(texts::proceed_line_1, texts::proceed_line_2);
		bool is_confirm_sensor_setup = choiceDialog(menu_timeout_secs);
		if (is_confirm_sensor_setup)
		{
			g_sensors->newSession();
		}

		// Show status screen
		displayStatus(false);
		return;
	}

	// Else reapeatedly add sensors.
	do
	{
		//Exit if adding another but max sensors reached
		bool can_add_sensor = g_sensors->canAddSensor();
		if (!can_add_sensor)
		{
			g_display->showAlertCenter(texts::setup_sensor_array_full_1, texts::setup_sensor_array_full_2);
			delay(display::standard_delay);
			break;
		}

		// Else show a waiting message while adding the senson
		g_display->showAlertCenter(texts::setup_sensors_waiting);
		bool was_sensor_added = g_sensors->pair();
		if (!was_sensor_added)
		{
			g_display->showAlertCenter(texts::setup_sensor_exists_1, texts::setup_sensor_exists_2);
			delay(display::standard_delay);
		}

		// Show menu again and wait choice
		g_display->showAlertStart(texts::setup_sensors_add_line_1, texts::setup_sensors_add_line_2);
		bool is_add_another = choiceDialog(menu_timeout_secs);

		// Exit if not adding another
		if (!is_add_another)
		{
			break;
		}
	} while (1);

	// Show status screen
	displayStatus(false);
}

/*
 * The user can see in the form of tabs, wifi information, setup sensors,
 * change the wifi network, change the pin or reboot the alarm system.
 */
void mainMenu()
{
	// Initial show of the first tab
	uint8_t current_tab = 0;
	g_display->showMenuTab(current_tab);
	// Timer and exit condition for exit
	Timer timer = Timer(menu_timeout_secs);
	bool exit = false;
	do
	{
		// Get a key press
		do
		{
			// Check for timeout
			if (timer.timeout())
			{
				exit = true;
			}
			g_key->getNew();
		} while (!g_key->enterPressed() && !g_key->nextPressed() &&
				 !g_key->prevPressed() && !exit);
		// If an appropriate key was selected, reset the timer
		g_sound->menuKeyTone();
		timer.reset();
		// If enter was pressed, do different stuff depending on the tab
		if (g_key->enterPressed())
		{
			switch (current_tab)
			{
			case 0: // Show WiFi info
			{
				g_display->showWifiSsid(g_network_info.ssid);
				g_display->showLocalIP(g_network_info.local_ip);
				break;
			}

			case 1: // Change wifi network
			{
				g_display->showAlertCenter(texts::proceed_line_1, texts::proceed_line_2);
				bool is_confirm_wifi_change = choiceDialog(menu_timeout_secs);
				if (is_confirm_wifi_change)
				{
					changeNetwork();
				}
				break;
			}

			case 2: // Change pin
			{
				g_display->showAlertCenter(texts::proceed_line_1, texts::proceed_line_2);
				bool is_confirm_pin_change = choiceDialog(menu_timeout_secs);
				if (is_confirm_pin_change)
				{
					String newPin = inputPin(false);
					if (newPin.length() >= data::pin_length)
					{
						changePin(newPin.c_str());
					}
				}
				break;
			}

			case 3: // Setup sensors
			{
				sensorSetup();
				break;
			}
			case 4: // Load defaults
			{
				g_display->showAlertCenter(texts::proceed_line_1, texts::proceed_line_2);
				bool is_confirm_load_defaults = choiceDialog(menu_timeout_secs);
				if (is_confirm_load_defaults)
				{
					changePin(default_pin);
					g_sound->successTone();
					g_display->showAlertCenter(texts::defaults_loaded);
					delay(display::standard_delay);
				}
				break;
			}
			case 5: // Reset
			{
				g_display->showAlertCenter(texts::proceed_line_1, texts::proceed_line_2);
				bool is_confirm_reset = choiceDialog(menu_timeout_secs);
				if (is_confirm_reset)
				{
					if (g_serial->sendReset())
					{
						resetFunc();
					}
				}
				break;
			}
			}
			// Exit the menu
			exit = true;
		}
		else
		{
			// Else a directional key was pressed, change the tab
			if (g_key->nextPressed())
			{
				if (current_tab < menu_tabs)
				{
					current_tab++;
				}
			}
			else if (g_key->prevPressed())
			{
				if (current_tab > 0)
				{
					current_tab--;
				}
				else
				{
					// If back is pressed from the first tab, exit
					exit = true;
				}
			}
			// Show the new tab
			g_display->showMenuTab(current_tab);
		}
	} while (!exit);
	// Display the status after the menu exits
	displayStatus(true);
}

/*
 * Returns false if the disconnected command is read via serial, or true
 * otherwise. If true is returned, the network info is also updated.
 */
bool isNetworkConnected()
{
	while (1)
	{
		// If command is not found
		if (g_serial->getCommand())
		{
			// If the disconnected command is found
			if (g_serial->readNetworkDisconnected())
			{
				g_sound->failureTone();
				g_display->showAlertCenter(texts::wifi_disconnect);
				delay(display::standard_delay);
				g_serial->clearSerial();
				return false;
			}
			// Get the net info and return true
			g_network_info = g_serial->readNetInfo();
			g_sound->successTone();
			g_display->showAlertCenter(texts::wifi_connected);
			delay(display::standard_delay);
			g_serial->clearSerial();
			return true;
		}
	}
}

/*
 * A wifi password up to 16 characters is expected from the user, which is then
 * returned. Pressing the same ABC button twice rotates the alphabet or the
 * symbols if pressed again in the next second. Numbers are displayed as
 * numbers.
 */
String insertNetworkPassword()
{
	g_display->showEnterWifiPass("");
	// Password index
	char password_buffer[network::max_credential_length + 1] = {0};
	uint8_t index = 0;
	// Chars to rotate
	char letter_small = 'a';
	char letter_caps = 'A';
	char symbol = '!';
	g_key->reset();
	// On timeout this timer resets the char to the first one
	Timer startover_timer(key_timeout_secs);
	while (1)
	{
		// Store the previous key, before getting a new one
		char previous_key = g_key->getCurrent();
		do
		{
			g_key->getNew();
		} while (g_key->noKeyPressed());
		g_sound->pinKeyTone();

		// If its accept, break the loop
		if (g_key->acceptPressed())
		{
			break;
		}

		// Backspace one character
		if (g_key->backspacePressed())
		{
			if (index > 0)
			{
				index--;
			}
			password_buffer[index] = 0;
		}
		else
		{
			// While the password is within allowed range
			if (index < network::max_credential_length)
			{
				// Small letter input
				if (g_key->aPressed())
				{
					if (previous_key == g_key->getCurrent() &&
						!startover_timer.timeout())
					{
						if (letter_small == 'z')
						{
							letter_small = 'a';
						}
						else
						{
							letter_small++;
						}
						if (index > 0)
						{
							index--;
						}
					}
					else
					{
						letter_small = 'a';
					}
					password_buffer[index] = letter_small;
					startover_timer.reset();
				}
				// Caps input
				else if (g_key->bPressed())
				{
					if (previous_key == g_key->getCurrent() &&
						!startover_timer.timeout())
					{
						if (letter_caps == 'Z')
						{
							letter_caps = 'A';
						}
						else
						{
							letter_caps++;
						}

						if (index > 0)
						{
							index--;
						}
					}
					else
					{
						letter_caps = 'A';
					}
					password_buffer[index] = letter_caps;
					startover_timer.reset();
				}
				// Symbol input
				else if (g_key->cPressed())
				{
					if (previous_key == g_key->getCurrent() &&
						!startover_timer.timeout())
					{
						switch (symbol)
						{
						case '/':
							symbol = ':';
							break;
						case '@':
							symbol = '[';
							break;
						case '`':
							symbol = '{';
							break;
						case '}':
							symbol = '!';
							break;
						default:
							symbol++;
							break;
						}
						if (index > 0)
							index--;
					}
					else
					{
						symbol = '!';
					}
					password_buffer[index] = symbol;
					startover_timer.reset();
				}
				// Number input
				else
				{
					password_buffer[index] = g_key->getCurrent();
				}
				index++;
			}
		}
		g_display->showEnterWifiPass(password_buffer);
	}
	return String(password_buffer);
}

/* 
 * Returns true for a successful connection with a wifi network, which happens
 * in collaboration with the serial class. After the esp receives a request to
 * change the network a network list is returned and ram gets allocated for a
 * network list, the size of which depends in the currently free ram. After the
 * user chooses a network and types the password an attempt for connection is
 * made. If the attempt is successful the function returns true or false
 * otherwise.
 */
bool connectNewNetwork()
{
	g_display->showScanWifi();
	while (!g_serial->getCommand())
		;
	int8_t network_count = g_serial->readNetworkHeader();
	g_serial->clearSerial();
	// If there are available networks
	if (network_count > 0)
	{
		// Dynamically allocate a list of networks
		uint8_t array_size = 10;
		network::ScannedNetwork *networks = (network::ScannedNetwork *)malloc(
			array_size * sizeof(network::ScannedNetwork));
		if (array_size < network_count)
		{
			network_count = array_size;
		}
		// Read each network until the end command
		uint8_t index = 0;
		bool done = false;
		do
		{
			if (g_serial->getCommand())
			{
				if (g_serial->readNetworkEnd())
				{
					done = true;
				}
				else
				{
					network::ScannedNetwork network = g_serial->readNetwork();
					// If more networks than allowed are received, replace the networks
					// with weaker signal
					if (index >= network_count)
					{
						uint8_t min_rssi_index = 0;
						for (uint8_t i = 0; i < array_size; i++)
						{
							if (networks[min_rssi_index].rssi < networks[i].rssi)
							{
								min_rssi_index = i;
							}
						}
						if (networks[min_rssi_index].rssi < network.rssi)
						{
							networks[min_rssi_index] = network;
						}
					}
					// Otherwise all the network to the list
					else
					{
						networks[index] = network;
						index++;
					}
				}
				g_serial->clearSerial();
			}
		} while (!done);

		// Make a menu from the networks for the user to choose
		index = 0;
		while (1)
		{
			// Display network depending on index
			bool is_first = index == 0 ? true : false;
			bool is_last = (network_count - 1) == index ? true : false;
			g_display->showWifiNetwork(networks[index].ssid, networks[index].rssi,
									   is_first, is_last);
			// Listen for a key
			do
			{
				g_key->getNew();
			} while (!g_key->enterPressed() && !g_key->nextPressed() &&
					 !g_key->prevPressed() && !g_key->rescanPressed());
			g_sound->menuKeyTone();

			// If enter is pressed
			if (g_key->enterPressed())
			{
				// Display the network encryption
				g_display->showWifiEncryption(networks[index].encryption);
				// Get a password from the user
				char pass_buffer[network::max_credential_length + 1] = {0};
				if (networks[index].encryption != network::encrytpion_none)
				{
					strncpy(pass_buffer, insertNetworkPassword().c_str(),
							network::max_credential_length);
				}
				// Send the credentials
				while (!g_serial->sendNetCredentials(networks[index].ssid, pass_buffer))
					;
				g_display->showWifiSsid(networks[index].ssid);
				g_display->showAlertCenter(texts::wifi_connecting);
				delay(display::standard_delay);
				// Either get wifi info or a not connected message
				if (isNetworkConnected())
				{
					free(networks);
					return true;
				}
				else
				{
					free(networks);
					return false;
				}
			}
			else if (g_key->nextPressed())
			{
				if (index < network_count - 1)
				{
					index++;
				}
			}
			else if (g_key->prevPressed())
			{
				if (index > 0)
				{
					index--;
				}
			}
			else if (g_key->rescanPressed())
			{
				free(networks);
				return false;
			}
		}
	}
	g_display->showAlertCenter(texts::no_networks_line_1, texts::no_networks_line_2);
	delay(display::standard_delay);
	return false;
}

/* 
 * Used for reconnecting to a network, it presents 2 choices, to connect to a
 * new network or a retry attempt, which is selected by default if the user
 * doesn't submit an answer before timeout.
 */
bool reconnectNetwork()
{
	g_display->resetBacklightTimer();
	g_display->showAlertCenter(texts::wifi_disconnect);
	delay(display::standard_delay);
	g_display->showAlertStart(texts::wifi_select_line_1, texts::wifi_select_line_2);
	bool is_change_option = choiceDialog(selection_timeout_secs);
	if (is_change_option)
	{
		bool is_changed = changeNetwork();
		return is_changed;
	}
	else
	{
		g_display->showAlertCenter(texts::wifi_connecting);
		delay(display::standard_delay);
		g_serial->sendRetry();
		// Either get wifi info or a not connected message
		return isNetworkConnected();
	}
}

// Returns the ammound of free ram in the system.
uint16_t getFreeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	int ram = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
	return ram;
}