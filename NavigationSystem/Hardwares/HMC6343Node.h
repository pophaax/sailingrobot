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

#include "Database/DBHandler.h"
#include "Hardwares/i2ccontroller/I2CController.h"
#include "MessageBus/ActiveNode.h"

// Magic numbers correspond to the compass commands, see the datasheet for more info.
enum class CompassOrientation {
    COM_ORIENT_LEVEL = 0x72,
    COM_ORIENT_SIDEWAYS = 0x73,
    COM_ORIENT_FLATFRONT = 0x74
};

enum class CompassMeasurementRate {
    COM_MEASUREMENT_RATE_1HZ  = 0x00,
    COM_MEASUREMENT_RATE_5HZ  = 0x01, // hardware default
    COM_MEASUREMENT_RATE_10HZ = 0x02,
    COM_MEASUREMENT_RATE_NA   = 0x03
};

class HMC6343Node : public ActiveNode {
   public:
    HMC6343Node(MessageBus& msgBus, DBHandler& dbhandler);

    virtual ~HMC6343Node() {}

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

    ///----------------------------------------------------------------------------------
    /// Sets the compass's measurement rate.
    ///----------------------------------------------------------------------------------
    bool setMeasurementRate(CompassMeasurementRate rate);

    ///----------------------------------------------------------------------------------
    /// Reads the heading, pitch and roll from the compass.
    ///----------------------------------------------------------------------------------
    bool readData(float& heading, float& pitch, float& roll);
    bool readData(float& heading, float& pitch, float& roll, uint64_t& timestamp);

    void calibrate(int calibrationTime);

   protected:
    ///----------------------------------------------------------------------------------
    /// Reads the I2C Compass's state every x milliseconds, see COMPASS_SENSOR_SLEEP_MS
    /// for information on the timing in HMC6343Node.cpp. The state is then published to
    /// the message bus
    ///----------------------------------------------------------------------------------
    static void HMC6343ThreadFunc(ActiveNode* nodePtr);

    ///----------------------------------------------------------------------------------
    /// Update values from the database as the loop time of the thread and others parameters
    ///----------------------------------------------------------------------------------
    void updateConfigsFromDB();

    I2CController m_I2C;
    bool m_Initialised;
    int m_HeadingBufferSize;  // Number of byte describing the compass data
    double m_LoopTime;        // in seconds
    DBHandler& m_db;

    // HMC6343 Orientations
    const int LEVEL = 0;      // X = forward, +Z = up (default)
    const int SIDEWAYS = 1;   // X = forward, +Y = up
    const int FLATFRONT = 2;  // Z = forward, -X = up
};
