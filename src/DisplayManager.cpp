#include "DisplayManager.h"
#include "texts.h"

display::DisplayManager *display::DisplayManager::m_instance = nullptr;

display::DisplayManager *display::DisplayManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new display::DisplayManager();
	}
	return m_instance;
}

display::DisplayManager::DisplayManager()
{
	m_lcd = new LiquidCrystal_I2C(lcd_address, en_pin, rw_pin, rs_pin, d4_pin, d5_pin, d6_pin, d7_pin, backlight_pin, POSITIVE);
}

//In begin the lcd object is set up, first by calling lcd's begin,
//then setting backlight to high and then creating custom symbols
//for the wifi representation in the form of 8 lines with 5 columns each.
//Setting a binary value of 0 or 1 in that column lights the pixel white
//or leaves it non lit. For a more reprehensive represantation 2 lcd columns
//are used to paint a wifi signal strength bar resembling a mobile phone's
//GSM signal bars.
void display::DisplayManager::init()
{
	m_lcd->begin(lcd_columns, lcd_lines);
	m_lcd->setBacklight(HIGH);
	//Character byte arrays
	uint8_t max_signal_pt1[8] = {B00000, B00000, B00000, B00000, B00001, B00001, B01001, B01001};
	uint8_t max_singnal_pt2[8] = {B00001, B00001, B01001, B01001, B01001, B01001, B01001, B01001};
	uint8_t weak_signal_pt1[8] = {B00000, B00000, B00000, B00000, B00001, B00000, B01000, B01001};
	uint8_t weak_signal_pt2[8] = {B00001, B00000, B01000, B01000, B01000, B01000, B01000, B01001};
	uint8_t no_signal_pt1[8] = {B00000, B00000, B00000, B00000, B00001, B00000, B00000, B01001};
	uint8_t no_signal_pt2[8] = {B00001, B00000, B01000, B00000, B00000, B00000, B00000, B01001};
	//Create the above characters
	m_lcd->createChar(1, max_signal_pt1);
	m_lcd->createChar(2, max_singnal_pt2);
	m_lcd->createChar(3, weak_signal_pt1);
	m_lcd->createChar(4, weak_signal_pt2);
	m_lcd->createChar(5, no_signal_pt1);
	m_lcd->createChar(6, no_signal_pt2);
}

//A timer is used for the backlight timeout, when reset
//the backlight is turned on and the timer is reset.
void display::DisplayManager::resetBacklightTimer()
{
	m_lcd->backlight();
	m_backlight_timer.reset();
}

//If the timeout time has passed without a reset then the
//backlight is turned off.
void display::DisplayManager::turnOffBacklight()
{
	if (m_backlight_timer.timeout())
	{
		m_lcd->noBacklight();
	}
}

//Displays status of alarm based on the state, at both lines of the lcd.
void display::DisplayManager::showStatus(uint8_t state, int32_t rssi, int8_t magnet_count, int8_t pir_count)
{
	m_lcd->clear();
	switch (state)
	{
	case 0:
		m_lcd->print(texts::getFlashString(texts::disarmed));
		m_lcd->setCursor(10, 0);
		m_lcd->print(texts::getFlashString(texts::wifi));
		addWifiSignal(rssi); //RSSI value is tested for integrity inside showWifiSignal
		m_lcd->setCursor(0, 1);
		m_lcd->print(texts::getFlashString(texts::menu));
		m_lcd->setCursor(10, 1);
		m_lcd->print(texts::getFlashString(texts::pin));
		break;
	case 1:
		m_lcd->print(texts::getFlashString(texts::armed));
		m_lcd->setCursor(10, 0);
		m_lcd->print(texts::getFlashString(texts::wifi));
		addWifiSignal(rssi); //RSSI value is tested for integrity inside showWifiSignal
		addSensorCount(magnet_count, pir_count);
		break;
	case 2:
		m_lcd->clear();
		m_lcd->print(texts::getFlashString(texts::alert_triggered));
		break;
	}
}

//Adds a sensor display on second row of lcd.
void display::DisplayManager::addSensorCount(int8_t magnet_count, int8_t pir_count)
{
	m_lcd->setCursor(0, 1);
	if (magnet_count >= 0)
	{
		m_lcd->print(texts::getFlashString(texts::magnet));
		m_lcd->print(magnet_count);
	}
	if (pir_count >= 0)
	{
		m_lcd->print(texts::getFlashString(texts::pir));
		m_lcd->print(pir_count);
	}
}

//Displays a menu tab depending on the tab number at both lines of the lcd.
void display::DisplayManager::showMenuTab(uint8_t current_tab)
{
	m_lcd->clear();
	switch (current_tab)
	{
	case 0:
		printFlashTextCenter(texts::menu_wifi_info);
		break;
	case 1:
		printFlashTextCenter(texts::menu_change_wifi);
		break;
	case 2:
		printFlashTextCenter(texts::menu_change_pin);
		break;
	case 3:
		printFlashTextCenter(texts::setup_sensors);
		break;
	case 4:
		printFlashTextCenter(texts::menu_load_defaults);
		break;
	case 5:
		printFlashTextCenter(texts::menu_reset);
		break;
	default:
		m_lcd->print("UNKNOWN TAB");
		break;
	}
	m_lcd->setCursor(0, 1);
	m_lcd->print(left_arrow_symbol);
	if (current_tab < max_menu_tabs)
	{
		m_lcd->print(texts::getFlashString(texts::menu_keys));
		m_lcd->print(right_arrow_symbol);
	}
	else
	{
		m_lcd->print(texts::getFlashString(texts::menu_keys_last));
	}
}

void display::DisplayManager::showAlertStart(const char *line_1)
{
	m_lcd->clear();
	m_lcd->print(texts::getFlashString(line_1));
}

void display::DisplayManager::showAlertStart(const char *line_1, const char *line_2)
{
	m_lcd->clear();
	m_lcd->print(texts::getFlashString(line_1));
	m_lcd->setCursor(0, 1);
	m_lcd->print(texts::getFlashString(line_2));
}

void display::DisplayManager::showAlertCenter(const char *line_1)
{
	m_lcd->clear();
	printFlashTextCenter(line_1);
}

void display::DisplayManager::showAlertCenter(const char *line_1, const char *line_2)
{
	m_lcd->clear();
	printFlashTextCenter(line_1);
	m_lcd->setCursor(0, 1);
	printFlashTextCenter(line_2);
}

//Displays the sensor id as offline
void display::DisplayManager::showSensorNotification(const char *message, int sensor_id)
{
	m_lcd->clear();
	printFlashTextCenter(texts::sensor_x);
	//Move one to the left and overrrite the last placeholder char
	m_lcd->moveCursorLeft();
	m_lcd->print(sensor_id);
	//Change line
	m_lcd->setCursor(0, 1);
	//Print rest of the message
	printFlashTextCenter(message);
}

//Displays pin input at both lines of lcd. Characters appear as asteriscs.
void display::DisplayManager::showEnterPin(const char *pin)
{
	m_lcd->clear();
	m_lcd->print(texts::getFlashString(texts::enter_pin));
	uint8_t current_pin_length = strlen(pin);
	for (uint8_t i = 0; i < current_pin_length; i++)
	{
		m_lcd->print('*');
	}
	showInputOptions();
}

//Displays new pin input at both lines of the lcd. Pin characters are displayed normally.
void display::DisplayManager::showEnterNewPin(const char *pin)
{
	m_lcd->clear();
	m_lcd->print(texts::getFlashString(texts::new_pin));
	m_lcd->print(pin);
	showInputOptions();
	m_lcd->setCursor(9, 0);
}
//Displays an alarm arm delay message at both lines of the lcd.
void display::DisplayManager::showArmDelay(uint8_t seconds)
{
	m_lcd->clear();
	m_lcd->print(seconds);
	m_lcd->print(texts::getFlashString(texts::arm_delay_line_1));
	m_lcd->setCursor(0, 1);
	m_lcd->print(texts::getFlashString(texts::arm_delay_line_2));
}

//Displays a scanned wifi network info with options at both lines of the lcd,
//based on the position of the network in the network list(first, last or middle).
void display::DisplayManager::showWifiNetwork(const char *ssid, int32_t rssi, bool is_first_network, bool is_last_network)
{
	m_lcd->clear();
	m_lcd->print(ssid);
	m_lcd->setCursor(14, 0);
	addWifiSignal(rssi);
	m_lcd->setCursor(0, 1);
	if (is_first_network && is_last_network)
	{
		printFlashTextCenter(texts::wifi_connect_keys_single);
	}
	else if (is_first_network)
	{
		m_lcd->print(texts::getFlashString(texts::wifi_connect_keys_first));
		m_lcd->print(right_arrow_symbol);
	}
	else if (is_last_network)
	{
		m_lcd->print(left_arrow_symbol);
		m_lcd->print(texts::getFlashString(texts::wifi_connect_keys_last));
	}
	else
	{
		m_lcd->print(left_arrow_symbol);
		m_lcd->print(texts::getFlashString(texts::wifi_connect_keys));
		m_lcd->print(right_arrow_symbol);
	}
}

//Displays the encryption of the wifi network that the user
//is trying to connect, at both lines of the lcd.
void display::DisplayManager::showWifiEncryption(uint8_t encryption)
{
	m_lcd->clear();
	bool has_encryption = true;
	m_lcd->print(texts::getFlashString(texts::encryption));
	switch (encryption)
	{
	case 2:
		m_lcd->print(texts::getFlashString(texts::encr_wpa));
		break;
	case 4:
		m_lcd->print(texts::getFlashString(texts::encr_wpa2));
		break;
	case 5:
		m_lcd->print(texts::getFlashString(texts::encr_wep));
		break;
	case 7:
		has_encryption = false;
		m_lcd->print(texts::getFlashString(texts::encr_none));
		break;
	case 8:
		m_lcd->print(texts::getFlashString(texts::encr_auto));
		break;
	}
	m_lcd->setCursor(0, 1);
	if (has_encryption)
	{
		m_lcd->print(texts::getFlashString(texts::character_limit));
	}
	delay(extended_delay);
}

//Displays the ssid of a network at both lines of the lcd,
//followed by a delay.
void display::DisplayManager::showWifiSsid(const char *ssid)
{
	m_lcd->clear();
	printFlashTextCenter(texts::ssid);
	m_lcd->setCursor(0, 1);
	centerPrintText(ssid);
	delay(extended_delay);
}

//Display the local ip at both lines of the lcd, followed by a delay.
void display::DisplayManager::showLocalIP(const char *ip)
{
	m_lcd->clear();
	printFlashTextCenter(texts::local_ip);
	m_lcd->setCursor(0, 1);
	centerPrintText(ip);
	delay(extended_delay);
}

//Displays the wifi pass entered by the user at both lines of the lcd.
void display::DisplayManager::showEnterWifiPass(const char *pass)
{
	m_lcd->clear();
	m_lcd->print(pass);
	m_lcd->setCursor(0, 1);
	m_lcd->print(texts::getFlashString(texts::wifi_pass_keys));
	m_lcd->setCursor(0, 0);
}

//Display a wifi scan message at both lines of the lcd, followed by a delay.
void display::DisplayManager::showScanWifi()
{
	m_lcd->clear();
	m_lcd->print(texts::getFlashString(texts::wifi_scanning));
	m_lcd->setCursor(0, 1);
	printFlashTextCenter(texts::wifi_rescan);
	delay(standard_delay);
}

//Prints flash text in the center of the lcd row.
void display::DisplayManager::printFlashTextCenter(const char *texts)
{
	uint8_t text_length = strlen_P(texts);
	uint8_t spaces = (lcd_columns - text_length) / 2;
	for (uint8_t i = 0; i < spaces; i++)
	{
		m_lcd->print(' ');
	}
	m_lcd->print(texts::getFlashString(texts));
}

//Prints plain char text in the center of the lcd row.
void display::DisplayManager::centerPrintText(const char *texts)
{
	uint8_t text_length = strlen(texts);
	uint8_t spaces = (lcd_columns - text_length) / 2;
	for (uint8_t i = 0; i < spaces; i++)
	{
		m_lcd->print(' ');
	}
	m_lcd->print(texts);
}

//Display options for pin input, at the line 2 of the lcd.
void display::DisplayManager::showInputOptions()
{
	m_lcd->setCursor(0, 1);
	m_lcd->print(texts::getFlashString(texts::pin_keys));
}

//Displays wifi signal symbol with two custom chars, depending on the RSSI number.
void display::DisplayManager::addWifiSignal(int32_t dbm)
{
	if (dbm >= -30)
	{
		m_lcd->write(uint8_t(1));
		m_lcd->write(uint8_t(2));
	}
	else if (dbm >= -67)
	{
		m_lcd->write(uint8_t(1));
		m_lcd->write(uint8_t(4));
	}
	else if (dbm >= -70)
	{
		m_lcd->write(uint8_t(1));
		m_lcd->write(uint8_t(6));
	}
	else if (dbm >= -80)
	{
		m_lcd->write(uint8_t(3));
		m_lcd->write(uint8_t(6));
	}
	else
	{
		m_lcd->write(uint8_t(5));
		m_lcd->write(uint8_t(6));
	}
}