#include "KeyManager.h"

keypad::KeyManager *keypad::KeyManager::m_instance = nullptr;

keypad::KeyManager *keypad::KeyManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new keypad::KeyManager();
	}
	return m_instance;
}

keypad::KeyManager::KeyManager()
{
	m_current_key = key_none;
	resetPressedKeys();
	m_keypad = new i2ckeypad(i2c_address, rows, columns);
}

void keypad::KeyManager::reset()
{
	m_current_key = key_none;
	resetPressedKeys();
}

//Gets a new key and toggles the flag or flags
//related with that key.
void keypad::KeyManager::getNew()
{
	m_current_key = m_keypad->get_key();
	resetPressedKeys();
	switch (m_current_key)
	{
	case key_none:
		break;
	case key_a:
		setKeyAPressed();
		break;
	case key_b:
		setKeyBPressed();
		break;
	case key_c:
		setKeyCPressed();
		break;
	case key_d:
		setKeyDPressed();
		break;
	case key_hash:
		setHashKeyPressed();
		break;
	case key_star:
		setStarKeyPressed();
		break;
	default:
		setNumKeyPressed();
		break;
	}
}

//Gets the last pressed key in char type.
char keypad::KeyManager::getCurrent()
{
	return m_current_key;
}

//The functions below return true or false if
//the specific key related with that function
//was pressed or no key was pressed.
bool keypad::KeyManager::noKeyPressed()
{
	if (m_current_key == key_none)
	{
		return true;
	}
	return false;
}

bool keypad::KeyManager::numberPressed()
{
	return m_keys_pressed.number;
}

bool keypad::KeyManager::menuPressed()
{
	return m_keys_pressed.menu_key;
}

bool keypad::KeyManager::enterPressed()
{
	return m_keys_pressed.menu_enter;
}

bool keypad::KeyManager::nextPressed()
{
	return m_keys_pressed.menu_next;
}

bool keypad::KeyManager::prevPressed()
{
	return m_keys_pressed.menu_prev;
}

bool keypad::KeyManager::enterPinPressed()
{
	return m_keys_pressed.enter_pin;
}

bool keypad::KeyManager::rescanPressed()
{
	return m_keys_pressed.rescan;
}

bool keypad::KeyManager::aPressed()
{
	return m_keys_pressed.option_a;
}

bool keypad::KeyManager::bPressed()
{
	return m_keys_pressed.option_b;
}

bool keypad::KeyManager::cPressed()
{
	return m_keys_pressed.option_c;
}

bool keypad::KeyManager::dPressed()
{
	return m_keys_pressed.option_d;
}

bool keypad::KeyManager::acceptPressed()
{
	return m_keys_pressed.accept;
}

bool keypad::KeyManager::backspacePressed()
{
	return m_keys_pressed.backspace;
}

//Resets all key function flags.
void keypad::KeyManager::resetPressedKeys()
{
	m_keys_pressed.number = false;
	m_keys_pressed.menu_key = false;
	m_keys_pressed.menu_enter = false;
	m_keys_pressed.menu_next = false;
	m_keys_pressed.menu_prev = false;
	m_keys_pressed.enter_pin = false;
	m_keys_pressed.rescan = false;
	m_keys_pressed.option_a = false;
	m_keys_pressed.option_b = false;
	m_keys_pressed.option_c = false;
	m_keys_pressed.option_d = false;
	m_keys_pressed.accept = false;
	m_keys_pressed.backspace = false;
}

//The functions below set all the function flags related
//with the specific key to true.
void keypad::KeyManager::setKeyAPressed()
{
	m_keys_pressed.menu_key = true;
	m_keys_pressed.menu_next = true;
	m_keys_pressed.option_a = true;
}

void keypad::KeyManager::setKeyBPressed()
{
	m_keys_pressed.menu_prev = true;
	m_keys_pressed.option_b = true;
}

void keypad::KeyManager::setKeyCPressed()
{
	m_keys_pressed.menu_enter = true;
	m_keys_pressed.option_c = true;
}

void keypad::KeyManager::setKeyDPressed()
{
	m_keys_pressed.option_d = true;
	m_keys_pressed.enter_pin = true;
	m_keys_pressed.rescan = true;
}

void keypad::KeyManager::setHashKeyPressed()
{
	m_keys_pressed.accept = true;
}

void keypad::KeyManager::setStarKeyPressed()
{
	m_keys_pressed.backspace = true;
}

void keypad::KeyManager::setNumKeyPressed()
{
	m_keys_pressed.number = true;
}