/*
This class is in charge of the custom i2c keypad and its input using the library
i2ckeypad. There is key mapping and there is a flag system with the assigned roles of
a button, so when for example "A" is pressed all the roles associated with A
will be true. It also includes multiple boolean tests on the above flags in order
to simplify and hide the class' functionality and logical tests.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <i2ckeypad.h>

namespace keypad
{
	//Keymap of the keypad
	typedef enum key_t
	{
		key_hash = '#',
		key_star = '*',
		key_a = 'A',
		key_b = 'B',
		key_c = 'C',
		key_d = 'D',
		key_none = '\0'
	} key_t;

	//Flags of assigned button roles
	struct KeysPressed
	{
		bool number : 1,
			accept : 1,
			backspace : 1,
			menu_key : 1,
			menu_enter : 1,
			menu_next : 1,
			menu_prev : 1,
			enter_pin : 1,
			rescan : 1,
			option_a : 1,
			option_b : 1,
			option_c : 1,
			option_d : 1;
	};

	const uint8_t rows = 4;			  //Number of rows on the keypad
	const uint8_t columns = 4;		  //Number of columns on the keypad
	const uint8_t i2c_address = 0x20; //I2C address of the PCF8574 chip

	class KeyManager
	{
	public:
		KeyManager(KeyManager const &) = delete;
		void operator=(KeyManager const &) = delete;
		static KeyManager *getInstance();
		void reset();
		void getNew();
		char getCurrent();
		bool noKeyPressed();
		bool numberPressed();
		bool menuPressed();
		bool enterPressed();
		bool nextPressed();
		bool prevPressed();
		bool enterPinPressed();
		bool rescanPressed();
		bool aPressed();
		bool bPressed();
		bool cPressed();
		bool dPressed();
		bool acceptPressed();
		bool backspacePressed();

	private:
		//Methods
		KeyManager();
		void resetPressedKeys();
		void setKeyAPressed();
		void setKeyBPressed();
		void setKeyCPressed();
		void setKeyDPressed();
		void setHashKeyPressed();
		void setStarKeyPressed();
		void setNumKeyPressed();
		//Variables
		static KeyManager *m_instance;
		i2ckeypad *m_keypad;
		char m_current_key;
		keypad::KeysPressed m_keys_pressed;
	};
} // namespace keypad