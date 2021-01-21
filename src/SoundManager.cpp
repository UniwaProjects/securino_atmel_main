#include "SoundManager.h"

sound::SoundManager *sound::SoundManager::m_instance = nullptr;

sound::SoundManager *sound::SoundManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SoundManager();
	}
	return m_instance;
}

sound::SoundManager::SoundManager() {}

void sound::SoundManager::init(uint8_t buzzer_pin)
{
	m_buzzer_pin = buzzer_pin;
}

//Uses the non blocking function tone to sound the G note.
//This tone will be used for pin key presses.
void sound::SoundManager::pinKeyTone()
{
	tone(m_buzzer_pin, note_g, 250);
}

//Is used for menu key presses.
void sound::SoundManager::menuKeyTone()
{
	tone(m_buzzer_pin, note_a, 250);
}

//Tone used for correct actions. Delays are used
//so that the tone will not get interrupted by another
//key press.
void sound::SoundManager::successTone()
{
	uint16_t duration_millis = 200;
	tone(m_buzzer_pin, note_a, duration_millis);
	delay(duration_millis);
	tone(m_buzzer_pin, note_g, duration_millis);
	delay(duration_millis);
}

//Same as the above function for oppossite actions.
void sound::SoundManager::failureTone()
{
	uint16_t duration_millis = 400;
	tone(m_buzzer_pin, note_a, duration_millis);
	delay(duration_millis);
}

//Used when the system goes on alarm. Since the tone function
//without a duration will not stop on its own the noTone
//function is needed in order for it to stop.
void sound::SoundManager::alarm()
{
	tone(m_buzzer_pin, high_frequency);
}

//Used to stop the tone function called from the alarm function.
void sound::SoundManager::stopAlarm()
{
	noTone(m_buzzer_pin);
}
