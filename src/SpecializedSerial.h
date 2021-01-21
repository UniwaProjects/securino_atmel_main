/*
Inhrerits from the SerialManager class, shared between the Arduino and the ESP.
Includes the operations required for the Arduino part, handling sent and received
information via the Serial protocol.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "common/SerialManager.h"
#include "common/networktypes.h"

namespace serial
{
	class SpecializedSerial : public SerialManager
	{
	public:
		SpecializedSerial(SpecializedSerial const &) = delete;
		void operator=(SpecializedSerial const &) = delete;
		static SpecializedSerial *getInstance();
		bool sendNetChange();
		bool sendRetry();
		bool sendReset();
		bool sendNetCredentials(const char *ssid, const char *pass);
		network::Info readNetInfo();
		uint32_t readDeviceId();
		bool readNetworkDisconnected();
		int8_t readNetworkHeader();
		network::ScannedNetwork readNetwork();
		bool readNetworkEnd();

	private:
		//Methods
		SpecializedSerial();
		//Variables
		static SpecializedSerial *m_instance;
	};
} // namespace serial