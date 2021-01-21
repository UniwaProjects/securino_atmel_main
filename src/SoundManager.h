/*
This class is responsible for audio feedback using the buzzer. There are
different notes for different types of action, in order of actions to be auditably
recognizable.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

namespace sound
{

	const uint16_t note_g = 392;		  //G note's frequency
	const uint16_t note_a = 440;		  //A note's frequency
	const uint16_t high_frequency = 1500; //A high pitched frequency

	class SoundManager
	{
	public:
		SoundManager(SoundManager const &) = delete;
		void operator=(SoundManager const &) = delete;
		static SoundManager *getInstance();
		void init(uint8_t m_buzzer_pin);
		void pinKeyTone();
		void menuKeyTone();
		void successTone();
		void failureTone();
		void alarm();
		void stopAlarm();

	private:
		//Methods
		SoundManager();
		//Variables
		static SoundManager *m_instance;
		uint8_t m_buzzer_pin;
	};
} // namespace sound