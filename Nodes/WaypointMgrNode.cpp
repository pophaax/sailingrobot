/****************************************************************************************
 *
 * File:
 * 		WaypointNode.cpp
 *
 * Purpose:
 *		The WaypointNode sends information about the waypoints to the sailing logic
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#include "WaypointMgrNode.h"
#include "Messages/WaypointDataMsg.h"
#include "Messages/ServerWaypointsReceivedMsg.h"
#include "SystemServices/Logger.h"
#include "dbhandler/DBHandler.h"
#include "utility/Utility.h"
#include <string>
#include <vector>
#include <cmath>

WaypointMgrNode::WaypointMgrNode(MessageBus& msgBus, DBHandler& db)
: Node(NodeID::Waypoint, msgBus), m_db(db),
    m_nextId(0),
    m_nextLongitude(0),
    m_nextLatitude(0),
    m_nextDeclination(0),
    m_nextRadius(0),

    m_prevId(0),
    m_prevLongitude(0),
    m_prevLatitude(0),
    m_prevDeclination(0),
    m_prevRadius(0)
{
    msgBus.registerNode(*this, MessageType::GPSData);
    msgBus.registerNode(*this, MessageType::ServerWaypointsReceived);
}

bool WaypointMgrNode::init()
{
    sendMessage();
    return true;
}


void WaypointMgrNode::processMessage(const Message* msg)
{
	MessageType type = msg->messageType();

	switch(type)
	{
        case MessageType::GPSData:
            processGPSMessage((GPSDataMsg*)msg);
            break;
        case MessageType::ServerWaypointsReceived:
            sendMessage();
        default:
            return;
	}

    if(waypointReached())
    {
        sendMessage();
    }
}

void WaypointMgrNode::processGPSMessage(GPSDataMsg* msg)
{
    m_gpsLongitude = msg->longitude();
    m_gpsLatitude = msg->latitude();
}

bool WaypointMgrNode::waypointReached()
{
    // double distanceAfterWaypoint = Utility::calculateWaypointsOrthogonalLine(m_nextLongitude, m_nextLatitude, m_prevLongitude,
    //             m_prevLatitude, m_gpsLongitude, m_gpsLatitude); //Checks if boat has passed the waypoint following the line, without entering waypoints radius

	if(std::isnan(m_gpsLongitude) || std::isnan(m_gpsLatitude))
	{
		return false;
	}

    if(harvestWaypoint())
    {
        if(not m_db.changeOneValue("waypoints", std::to_string(m_nextId),"1","harvested"))
        {
            Logger::error("Failed to harvest waypoint");
        }
        Logger::info("Waypoint harvested");
        m_waypointTimer.stop();

        return true;
    }
    else
    {
        return false;
    }
}

void WaypointMgrNode::sendMessage()
{
    if(m_db.getWaypointValues(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius, m_nextStayTime,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius))
    {
        MessagePtr msg = std::make_unique<WaypointDataMsg>(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius, m_nextStayTime,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius);
        m_MsgBus.sendMessage(std::move(msg));
    }
    else
    {
        Logger::warning("%s No waypoint found, boat is using old waypoint data. No message sent.", __func__);
    }

    m_db.forceUnlock();
}

bool WaypointMgrNode::harvestWaypoint()
{
    double DTW = CourseMath::calculateDTW(m_gpsLongitude, m_gpsLatitude, m_nextLongitude, m_nextLatitude); //Calculate distance to waypoint
    //Logger::info("Distance to WP: %f Radius: %f", DTW, m_nextRadius);
    if(DTW > m_nextRadius)
    {
        return false;
    }

    if(m_nextStayTime > 0) //if next waypoint has a time to stay inside its radius, start the timer
    {
        m_waypointTimer.start();
        if(not writeTime)
        {
            Logger::info("Started waypoint timer. Stay at waypoint for: %d seconds", m_nextStayTime);
            writeTime = true;
        }

        if(m_waypointTimer.timeReached(m_nextStayTime)) //Check if boat has stayed the designated amount of time
        {
            Logger::info("Waypoint timer passed");
            writeTime = false;
            return true;
        }

        return false;   
    }
    else //if no timer for waypoint, harvest it
    {
        return true;
    }
}
