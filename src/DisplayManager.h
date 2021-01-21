/*
The role of this class is to manage all of the visual feedback displayed on the
i2c lcd as well as the timeout of the backlight. Handles formatting the text in the
center of the screen when needed and reading flash strings to display.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include "common/Timer.h"

namespace display
{

	const uint8_t lcd_address = 0x27; //I2C address of the lcd
	const uint8_t en_pin = 2;		  //Pin EN on the I2C chip
	const uint8_t rw_pin = 1;		  //Pin RW on the I2C chip
	const uint8_t rs_pin = 0;		  //Pin RS on the I2C chip
	const uint8_t d4_pin = 4;		  //Pin D4 on the I2C chip
	const uint8_t d5_pin = 5;		  //Pin D5 on the I2C chip
	const uint8_t d6_pin = 6;		  //Pin D6 on the I2C chip
	const uint8_t d7_pin = 7;		  //Pin D7 on the I2C chip
	const uint8_t backlight_pin = 3;  //Backlight pin on the I2C chip
	const uint8_t lcd_columns = 16;	  //Columns of the lcd module
	const uint8_t lcd_lines = 2;	  //Lines of the lcd module

	const uint16_t standard_delay = 800;	   //Delay of screen messages
	const uint16_t extended_delay = 3000;	   //Delay of screen messages
	const uint16_t backlight_timeout_secs = 5; //Timeout of lcd backlight
	const uint8_t max_menu_tabs = 5;

	const char right_arrow_symbol = 126; //ASCII number for right arrow
	const char left_arrow_symbol = 127;	 //ASCII number for left arrow

	class DisplayManager
	{
	public:
		DisplayManager(DisplayManager const &) = delete;
		void operator=(DisplayManager const &) = delete;
		static DisplayManager *getInstance();
		void init();
		void resetBacklightTimer();
		void turnOffBacklight();
		//Generic prints
		void showAlertStart(const char *line_1);
		void showAlertStart(const char *line_1, const char *line_2);
		void showAlertCenter(const char *line_1);
		void showAlertCenter(const char *line_1, const char *line_2);
		//Status messages
		void showStatus(uint8_t state, int32_t rssi, int8_t magnet_count, int8_t pir_count = -1);
		void addSensorCount(int8_t magnet_count, int8_t pir_count);
		//Menu messages
		void showMenuTab(uint8_t tab);
		// Pin/Pass messages
		void showEnterPin(const char *pin);
		void showEnterNewPin(const char *pin);
		//Sensor related messages
		void showArmDelay(uint8_t seconds);
		void showSensorNotification(const char *message, int sensor_id);
		// Wifi related messages
		void showWifiSsid(const char *ssid);
		void showLocalIP(const char *ip);
		void showWifiNetwork(const char *ssid, int32_t rssi, bool is_first_network, bool is_last_network);
		void showWifiEncryption(uint8_t encryption);
		void showEnterWifiPass(const char *pass);
		void showScanWifi();

	private:
		//Methods
		DisplayManager();
		void printFlashTextCenter(const char *texts);
		void centerPrintText(const char *texts);
		void addWifiSignal(int32_t dbm);
		void showInputOptions();
		//Variables
		static DisplayManager *m_instance;
		LiquidCrystal_I2C *m_lcd;
		Timer m_backlight_timer = Timer(backlight_timeout_secs);
	};
} // namespace display