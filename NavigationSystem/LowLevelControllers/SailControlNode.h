/****************************************************************************************
 *
 * File:
 * 		SailControlNode.h
 *
 * Purpose:
 *      Calculates the desired sail angle.
 *      It sends a SailComandMsg corresponding to the command angle of the sail.
 *
 * Developer Notes:
 *      Two functions have been developed to calculate the desired sail angle :
 *          - calculateSailAngleLinear(),
 *          - calculateSailAngleCardioid().
 *      You can choose the one you want to use by commenting/uncommenting lines
 *      in SailControlNodeThreadFunc().
 *
 ***************************************************************************************/
#pragma once

#include <math.h>
#include <stdint.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "Database/DBHandler.h"
#include "Math/Utility.h"
#include "MessageBus/ActiveNode.h"
#include "MessageBus/MessageBus.h"
#include "Messages/SailCommandMsg.h"
#include "Messages/WindDataMsg.h"
#include "SystemServices/Timer.h"

class SailControlNode : public ActiveNode {
   public:
    SailControlNode(MessageBus& msgBus, DBHandler& dbhandler);
    ~SailControlNode();

    bool init();
    void start();
    void stop();
    void processMessage(const Message* message);

   private:
    ///----------------------------------------------------------------------------------
    /// Updates the values of the parameters from the database.
    ///----------------------------------------------------------------------------------
    void updateConfigsFromDB();

    ///----------------------------------------------------------------------------------
    /// Stores apparent wind direction from a WindStateMsg.
    ///----------------------------------------------------------------------------------
    void processWindStateMessage(const WindStateMsg* msg);

    ///----------------------------------------------------------------------------------
    /// Limits the command sail angle between m_MaxSailAngle and m_MinSailAngle.
    ///----------------------------------------------------------------------------------
    float restrictSailAngle(float val);

    ///----------------------------------------------------------------------------------
    /// Calculate the sail angle according to a linear relation to the apparent wind direction.
    ///----------------------------------------------------------------------------------
    float calculateSailAngleLinear();

    ///----------------------------------------------------------------------------------
    /// Calculate the sail angle according to a cardioid relation to the apparent wind direction.
    ///----------------------------------------------------------------------------------
    float calculateSailAngleCardioid();

    ///----------------------------------------------------------------------------------
    /// Starts the SailControlNode's thread that pumps out SailCommandMsg.
    ///----------------------------------------------------------------------------------
    static void SailControlNodeThreadFunc(ActiveNode* nodePtr);

    DBHandler& m_db;
    std::mutex m_lock;
    std::atomic<bool> m_Running;

    double m_LoopTime;      // seconds
    double m_MaxSailAngle;  // degrees
    double m_MinSailAngle;  // degrees

    double m_ApparentWindDir;  // degrees [0, 360[ in North-East reference frame (clockwise)
};
