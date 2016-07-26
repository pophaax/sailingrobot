/****************************************************************************************
 *
 * File:
 * 		HMC6343Node.h
 *
 * Purpose:
 *		The HMC6343 node communicates with a HMC6343 compass and provides CompassDataMsgs
 *		to the message bus.
 *
 * Developer Notes:
 * 		I2C Address
 *
 * 		The I2C address is different from what is mentioned in the datasheet. The
 * 		datasheet mentions it is 0x32, however when accessing the device you need to use
 * 		0x19 for successful communications. The register on the HMC6343 that contains the
 * 		I2C address returns 0x32, the address mentioned in the datasheet.
 *
 *
 ***************************************************************************************/

#pragma once


#include "ActiveNode.h"
#include "i2ccontroller/I2CController.h"

// Magic numbers correspond to the compass commands, see the datasheet for more info.
enum class CompassOrientation {
	COM_ORIENT_LEVEL = 0x72,
	COM_ORIENT_SIDEWAYS = 0x73,
	COM_ORIENT_FLATFRONT = 0x74
};


class HMC6343Node : public ActiveNode {
public:
	HMC6343Node(MessageBus& msgBus, const int headingBufferSize = 10);

	///----------------------------------------------------------------------------------
	/// Attempts to connect to the HMC6343 compass.
	///
	/// @returns		Returns false if it is unable to read the compass's own I2C
	///					address from its EPROM.
	///----------------------------------------------------------------------------------
	bool init();

	///----------------------------------------------------------------------------------
 	/// This function should be used to start the active nodes thread.
 	///----------------------------------------------------------------------------------
	void start();

	///----------------------------------------------------------------------------------
	/// Doesn't process any messages.
	///----------------------------------------------------------------------------------
	void processMessage(const Message* msg);

	///----------------------------------------------------------------------------------
	/// Sets the compass's orientation.
	///----------------------------------------------------------------------------------
	bool setOrientation(CompassOrientation orientation);

private:
	///----------------------------------------------------------------------------------
	/// Reads the heading, pitch and roll from the compass.
	///----------------------------------------------------------------------------------
	bool readData(float& heading, float& pitch, float& roll);

	///----------------------------------------------------------------------------------
	/// Reads the I2C Compass's state every x milliseconds, see COMPASS_SENSOR_SLEEP_MS
	/// for information on the timing in HMC6343Node.cpp. The state is then published to
	/// the message bus
	///----------------------------------------------------------------------------------
	static void HMC6343ThreadFunc(void* nodePtr);

	I2CController 	m_I2C;
	bool 			m_Initialised;
	const int		m_HeadingBufferSize;
};