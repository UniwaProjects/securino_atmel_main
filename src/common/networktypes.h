#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

namespace network
{
	//The following structs and enums are essentials for the exchange of Wifi information
	//in the form of organized objects and not magic numbers.
	const uint8_t max_credential_length = 16;
	typedef struct
	{
		char ssid[max_credential_length + 1];
		char pass[max_credential_length + 1];
	} Credentials;

	const uint8_t max_ip_length = 15; //XXX.XXX.XXX.XXX, 12 numbers, 3 dots
	typedef struct
	{
		char ssid[max_credential_length + 1];
		int32_t rssi;
		char local_ip[max_ip_length + 1];
	} Info;

	typedef enum encryption_t
	{
		//Values map to 802.11 encryption suites
		encryption_wep = 5,
		encrytpion_tkip = 2,
		encrytpion_ccmp = 4,
		//except these two, 7 and 8 are reserved in 802.11-2007
		encrytpion_none = 7,
		encrytpion_auto = 8
	} encryption_t;

	typedef struct
	{
		char ssid[max_credential_length + 1];
		int32_t rssi;
		encryption_t encryption;
	} ScannedNetwork;
} // namespace network
