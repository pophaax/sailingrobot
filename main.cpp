
#include <string>
#include "SystemServices/Logger.h"
#include "MessageBus/MessageBus.h"
#include "Nodes/MessageLoggerNode.h"
#include "Nodes/ActuatorNode.h"

#if SIMULATION == 1
 #include "Nodes/SimulationNode.h"
#else
 #include "Nodes/CV7Node.h"
 #include "Nodes/HMC6343Node.h"
 #include "Nodes/GPSDNode.h"
 #include "Nodes/ActuatorNode.h"
 #include "Nodes/ArduinoNode.h"
#endif

#define DISABLE_LOGGING 0

enum class NodeImportance {
	CRITICAL,
	NOT_CRITICAL
};

#if TARGET == 0
#define TARGET_STR "Default"

#include "Nodes/WaypointMgrNode.h"
#include "Nodes/VesselStateNode.h"
#include "Nodes/HTTPSyncNode.h"
#include "Nodes/XbeeSyncNode.h"
#include "Nodes/RoutingNode.h"
#include "Nodes/LineFollowNode.h"
#include "dbhandler/DBHandler.h"
#include "SystemServices/MaestroController.h"

#elif TARGET == 1
#define TARGET_STR "WRSC2016"

#include "Nodes/UDPNode.h"
#include "Nodes/GPSDNode.h"
#include "Nodes/SerialGPSNode.h"
#include "Nodes/MA3WindSensorNode.h"
#include "Nodes/RazorCompassNode.h"

#include "Nodes/WaypointMgrNode.h"
#include "Nodes/VesselStateNode.h"
#include "Nodes/RoutingNode.h"
#include "Nodes/LineFollowNode.h"
#include "dbhandler/DBHandler.h"
#include "SystemServices/MaestroController.h"

#include "Messages/ActuatorPositionMsg.h"

RazorCompassNode* razorFix;

#elif TARGET == 2
#define TARGET_STR "MANCONTROL"

#include "Nodes/UDPNode.h"
#include "Nodes/GPSDNode.h"
#include "Nodes/MA3WindSensorNode.h"
#include "Nodes/RazorCompassNode.h"
#include "Nodes/ManualControlNode.h"

#include "Nodes/WaypointMgrNode.h"
#include "Nodes/VesselStateNode.h"
#include "Nodes/RoutingNode.h"
#include "Nodes/LineFollowNode.h"
#include "dbhandler/DBHandler.h"
#include "SystemServices/MaestroController.h"

RazorCompassNode* razorFix;

#else
#define TARGET_STR "None"
#endif


#include <signal.h>
#include <atomic>
#include <cstring>

#include "WRSC.h"

void got_signal(int)
{
#if TARGET == 1
	razorFix->shutdown();
#endif
	exit(0);
}


///----------------------------------------------------------------------------------
/// Initialises a node and shutsdown the program if a critical node fails.
///
/// @param node 			A pointer to the node to initialise
/// @param nodeName 		A string name of the node, for logging purposes.
/// @param importance 		Whether the node is a critcal node or not critical. If a
///							critical node fails to initialise the program will
///							shutdown.
///
///----------------------------------------------------------------------------------
void initialiseNode(Node& node, const char* nodeName, NodeImportance importance)
{
	if(node.init())
	{
		Logger::info("Node: %s - init\t[OK]", nodeName);
	}
	else
	{
		Logger::error("Node: %s - init\t\t[FAILED]", nodeName);

		if(importance == NodeImportance::CRITICAL)
		{
			Logger::error("Critical node failed to initialise, shutting down");
			Logger::shutdown();
			exit(1);
		}
	}
}

void setupDB(DBHandler& dbHandler)
{
	if(dbHandler.initialise())
	{
		Logger::info("Database init\t\t[OK]");
	}
	else
	{
		Logger::error("Database init\t\t[FAILED]");
		Logger::shutdown();
		exit(1);
	}
}

///----------------------------------------------------------------------------------
/// Entry point, can accept one argument containing a relative path to the database.
///
///----------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	// This is for eclipse development so the output is constantly pumped out.
	setbuf(stdout, NULL);

	struct sigaction sa;
	    memset( &sa, 0, sizeof(sa) );
	    sa.sa_handler = got_signal;
	    sigfillset(&sa.sa_mask);
	    sigaction(SIGINT,&sa,NULL);

	// Database Path
	std::string db_path;
	if (argc < 2) {
		db_path = "./asr.db";
	} else {
		db_path = std::string(argv[1]);
	}

	printf("================================================================================\n");
	printf("\t\t\t\tSailing Robot\n");
	printf("\n");
	printf("================================================================================\n");

	if (Logger::init()) {
		Logger::info("Built on %s at %s", __DATE__, __TIME__);
		Logger::info("Target: %s", TARGET_STR);
		Logger::info("\nLogger init\t\t[OK]");
	}
	else {
		Logger::error("Logger init\t\t[FAILED]");
	}

	MessageBus messageBus;
	DBHandler dbHandler(db_path);

	MessageLoggerNode msgLogger(messageBus);
	setupDB(dbHandler);

	// All active nodes will have a reference here so we can start them after ever node has been initialised
	std::vector<ActiveNode*> activeNodes;

	//---------------------------------------------------------------------------------------------
	// Use Simulator

#if SIMULATION == 1
	Logger::info("Using the simulator!");
	SimulationNode simulation(messageBus);
	initialiseNode(simulation, "Simulation Node", NodeImportance::CRITICAL);
	activeNodes.push_back(&simulation);
#endif

	//---------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------
	// Target: Default
#if TARGET == 0

	// No sensor nodes if we are using the simulator
#if SIMULATOR != 1
	// Common Sensor Nodes
	CV7Node windSensor(messageBus, dbHandler.retrieveCell("windsensor_config", "1", "port"), dbHandler.retrieveCellAsInt("windsensor_config", "1", "baud_rate"));
	HMC6343Node compass(messageBus, dbHandler.retrieveCellAsInt("buffer_config", "1", "compass"));
	GPSDNode gpsd(messageBus);
	ArduinoNode arduino(messageBus);

	activeNodes.push_back(&windSensor);
	activeNodes.push_back(&compass);
	activeNodes.push_back(&gpsd);
	activeNodes.push_back(&arduino);

	initialiseNode(windSensor, "Wind Sensor", NodeImportance::CRITICAL);
	initialiseNode(compass, "Compass", NodeImportance::CRITICAL);
	initialiseNode(gpsd, "GPSD Node", NodeImportance::CRITICAL);
	initialiseNode(arduino, "Arduino Node", NodeImportance::NOT_CRITICAL);
#endif

	// Network Nodes
	XbeeSyncNode xbee(messageBus, dbHandler);
	HTTPSyncNode httpsync(messageBus, &dbHandler, 0, false);

	activeNodes.push_back(&xbee);
	activeNodes.push_back(&httpsync);

	// Sailing Logic nodes
	VesselStateNode vessel(messageBus);
	WaypointMgrNode waypoint(messageBus, dbHandler);

	activeNodes.push_back(&vessel);

	Node* sailingLogic;
	bool usingLineFollow = (bool)(dbHandler.retrieveCellAsInt("sailing_robot_config", "1", "line_follow"));
	if(usingLineFollow)
	{
		sailingLogic = new LineFollowNode(messageBus, dbHandler);
	}
	else
	{
		sailingLogic = new RoutingNode(messageBus, dbHandler);
	}

	// Actuator Node

	int channel = dbHandler.retrieveCellAsInt("sail_servo_config", "1", "channel");
	int speed = dbHandler.retrieveCellAsInt("sail_servo_config", "1", "speed");
	int acceleration = dbHandler.retrieveCellAsInt("sail_servo_config", "1", "acceleration");

	ActuatorNode sail(messageBus, NodeID::SailActuator, channel, speed, acceleration);

	channel = dbHandler.retrieveCellAsInt("rudder_servo_config", "1", "channel");
	speed = dbHandler.retrieveCellAsInt("rudder_servo_config", "1", "speed");
	acceleration = dbHandler.retrieveCellAsInt("rudder_servo_config", "1", "acceleration");

	ActuatorNode rudder(messageBus, NodeID::RudderActuator, channel, speed, acceleration);
	MaestroController::init("dev/tty/ACM0");

	initialiseNode(xbee, "Xbee Sync Node", NodeImportance::NOT_CRITICAL);
	initialiseNode(httpsync, "Httpsync Node", NodeImportance::CRITICAL);

	initialiseNode(vessel, "Vessel State Node", NodeImportance::CRITICAL);
	initialiseNode(waypoint, "Waypoint Node", NodeImportance::CRITICAL);

	if(usingLineFollow)
	{
		initialiseNode(*sailingLogic, "LineFollow Node", NodeImportance::CRITICAL);
	}
	else
	{
		initialiseNode(*sailingLogic, "Routing Node", NodeImportance::CRITICAL);
	}

	initialiseNode(sail, "Sail Actuator", NodeImportance::CRITICAL);
	initialiseNode(rudder, "Rudder Actuator", NodeImportance::CRITICAL);
#endif
	//---------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------
	// Target: WRSC
#if TARGET == 1

	UDPNode udp(messageBus, "172.20.26.191", 4320);

	MaestroController::init("/dev/ttyACM0");

	// No sensor nodes if we are using the simulator
#if SIMULATION != 1
	//MaestroController::init("/dev/ttyACM0");

	MA3WindSensorNode windSensor(messageBus, 11);
#if BOAT_TYPE == BOAT_ENSTA_GRAND
	GPSDNode gps(messageBus);
	RazorCompassNode compass(messageBus, "/dev/ttyUSB1");
	ActuatorNode sail(messageBus, NodeID::SailActuator, 1, 0, 0);
	ActuatorNode rudder(messageBus, NodeID::RudderActuator, 2, 0, 0);
#elif BOAT_TYPE == BOAT_ENSTA_PETIT
	SerialGPSNode gps(messageBus);
	RazorCompassNode compass(messageBus,"/dev/ttyACM1");
	ActuatorNode sail(messageBus, NodeID::SailActuator, 1, 0, 0);
	ActuatorNode rudder(messageBus, NodeID::RudderActuator, 0, 0, 0);
#endif

	razorFix = &compass;

	activeNodes.push_back(&windSensor);
	activeNodes.push_back(&gps);
	activeNodes.push_back(&compass);

	initialiseNode(compass, "Compass Node", NodeImportance::CRITICAL);
	initialiseNode(windSensor, "Wind Sensor Node", NodeImportance::CRITICAL);
	initialiseNode(gps, "GPS Node", NodeImportance::CRITICAL);

	initialiseNode(sail, "Sail Actuator", NodeImportance::CRITICAL);
	initialiseNode(rudder, "Rudder Actuator", NodeImportance::CRITICAL);

#endif

	// Sailing Logic nodes
	VesselStateNode vessel(messageBus);
	WaypointMgrNode waypoint(messageBus, dbHandler);

	activeNodes.push_back(&vessel);

	Node* sailingLogic;
	bool usingLineFollow = true; //(bool)(dbHandler.retrieveCellAsInt("sailing_robot_config", "1", "line_follow"));
	if(usingLineFollow)
	{
		sailingLogic = new LineFollowNode(messageBus, dbHandler);
	}
	else
	{
		sailingLogic = new RoutingNode(messageBus, dbHandler);
	}

	initialiseNode(udp, "UDP Node", NodeImportance::CRITICAL);

	initialiseNode(vessel, "Vessel State Node", NodeImportance::CRITICAL);
	initialiseNode(waypoint, "Waypoint Node", NodeImportance::CRITICAL);

	initialiseNode(*sailingLogic, "Sailing Logic", NodeImportance::CRITICAL);
#endif
	//---------------------------------------------------------------------------------------------

    //---------------------------------------------------------------------------------------------
    // Target: MANCONTROL
#if TARGET == 2

    // No sensor nodes if we are using the simulator
#if SIMULATION != 1

#endif

	UDPNode udp(messageBus, "127.0.0.1", 4320);

	//MA3WindSensorNode windSensor(messageBus, 2);
	//GPSDNode gps(messageBus);
	RazorCompassNode compass(messageBus);
	//razorFix = &compass;

	// QUICK TEST
	/*float heading, pitch,roll;
	if(compass.parseData("#YPR=-155.73,-76.48,-129.51", heading, pitch, roll))
	{
		Logger::info("Data: %f %f %f", heading, pitch, roll);
	}*/

	//activeNodes.push_back(&windSensor);
	//activeNodes.push_back(&gps);
	activeNodes.push_back(&compass);

	// Sailing Logic nodes
	VesselStateNode vessel(messageBus);
	WaypointMgrNode waypoint(messageBus, dbHandler);

	activeNodes.push_back(&vessel);

	Node* sailingLogic;
	bool usingLineFollow = (bool)(dbHandler.retrieveCellAsInt("sailing_robot_config", "1", "line_follow"));
	if(usingLineFollow)
	{
		sailingLogic = new LineFollowNode(messageBus, dbHandler);
	}
	else
	{
		sailingLogic = new RoutingNode(messageBus, dbHandler);
	}

	// Actuator Node
	ActuatorNode sail(messageBus, NodeID::SailActuator, 1, 0, 0);
	ActuatorNode rudder(messageBus, NodeID::RudderActuator, 0, 0, 0);
	MaestroController::init("/dev/ttyACM0");

	ManualControlNode manualControl(messageBus);
	activeNodes.push_back(&manualControl);

	initialiseNode(udp, "UDP Node", NodeImportance::CRITICAL);
	initialiseNode(compass, "Compass Node", NodeImportance::CRITICAL);
	//initialiseNode(windSensor, "Wind Sensor Node", NodeImportance::CRITICAL);
	//initialiseNode(gps, "GPS Node", NodeImportance::CRITICAL);

	initialiseNode(vessel, "Vessel State Node", NodeImportance::CRITICAL);
	initialiseNode(waypoint, "Waypoint Node", NodeImportance::CRITICAL);
	initialiseNode(sail, "Sail Actuator", NodeImportance::CRITICAL);
	initialiseNode(rudder, "Rudder Actuator", NodeImportance::CRITICAL);
    initialiseNode(manualControl, "Manual Control", NodeImportance::CRITICAL);

#endif
    //---------------------------------------------------------------------------------------------

	// Initialise nodes
	initialiseNode(msgLogger, "Message Logger", NodeImportance::NOT_CRITICAL);

	for(unsigned int i = 0; i < activeNodes.size(); i++)
	{
		activeNodes[i]->start();
	}

	Logger::info("Message bus started!");

	// Test actuator Positions
	// Rudder and Sail Max
	//MessagePtr actuatorMsg = std::make_unique<ActuatorPositionMsg>(RUDDER_MAX_US, SAIL_MAX_US);
	//messageBus.sendMessage(std::move(actuatorMsg));

	// Middle
	//MessagePtr actuatorMsg = std::make_unique<ActuatorPositionMsg>(RUDDER_MID_US, 1500);
	//messageBus.sendMessage(std::move(actuatorMsg));

	// Min
	MessagePtr actuatorMsg = std::make_unique<ActuatorPositionMsg>(RUDDER_MIN_US, SAIL_MIN_US);
	messageBus.sendMessage(std::move(actuatorMsg));

	messageBus.run();

	Logger::shutdown();
	delete sailingLogic;
	exit(0);
}




// Purely for reference, remove once complete

/*xBeeSync* xbee_handle;


static void threadXBeeSyncRun() {
	xbee_handle->run();

	Logger::warning("Xbee Sync thread has exited");
}

static void threadGPSupdate() {
	try {
		gps_handle->run();
	}
	catch (const char * e) {
		std::cout << "ERROR while running static void threadGPSupdate()" << e << std::endl;
	}
}

static void threadHTTPSyncRun() {
	try {
		httpsync_handle->run();
	}
	catch (const char * error) {
		Logger::warning("Xbee Sync thread has exited");
	}
}

static void threadWindsensor() {
	windsensor_handle->run();
	std::cout << " * Windsensor thread exited." << std::endl;
}


int main(int argc, char *argv[]) {
	// This is for eclipse development so the output is constantly pumped out.
	setbuf(stdout, NULL);

	std::string path, db_name, errorLog;
	if (argc < 2) {
		path = "./";
		db_name = "asr.db";
		errorLog = "errors.log";
	} else if (argc == 3 ) {
		path = std::string(argv[1]);
		db_name = std::string(argv[2]);
		errorLog = "errors.log";
	}
  else {
		path = std::string(argv[1]);
		db_name = "asr.db";
		errorLog = "errors.log";
	}

	printf("================================================================================\n");
	printf("\t\t\t\tSailing Robot\n");
	printf("\n");
	printf("================================================================================\n");

	if (Logger::init()) {
		Logger::info("Built on %s at %s", __DATE__, __TIME__);
		Logger::info("Logger init 		[OK]");
	}
	else {
		Logger::info("Logger init 		[FAILED]");
	}

	// Default time
	ExternalCommand externalCommand("1970-04-10T10:53:15.1234Z",true,0,0);
	SystemState systemstate(
		SystemStateModel(
			GPSModel("",PositionModel(0,0),0,0,0,0),
			WindsensorModel(0,0,0),
			CompassModel(0,0,0,AccelerationModel(0,0,0) ),
			AnalogArduinoModel(0, 0, 0, 0),
			0,
			0
		)
	);

	DBHandler db(path+db_name);

	if(db.initialise())
	{
		Logger::info("Successful database connection established");
	}

	bool mockGPS = db.retrieveCellAsInt("mock","1","gps");
    bool mockWindsensor = db.retrieveCellAsInt("mock","1","windsensor");

	// Create main sailing robot controller
	int http_delay =  db.retrieveCellAsInt("httpsync_config", "1", "delay");
	bool removeLogs = db.retrieveCellAsInt("httpsync_config", "1", "remove_logs");

	httpsync_handle = new HTTPSync( &db, http_delay, removeLogs );

    SailingRobot sr_handle(&externalCommand, &systemstate, &db, httpsync_handle);

	GPSupdater gps_updater(&systemstate,mockGPS);
	gps_handle = &gps_updater;

	try {
		if( not sr_handle.init(path, errorLog) )
		{
			Logger::error("Failed to initialise SailingRobot, exiting...");
			return 1;
		}

		windsensor_handle.reset(
			new WindsensorController(
				&systemstate,
				mockWindsensor,
				db.retrieveCell("windsensor_config", "1", "port"),
				db.retrieveCellAsInt("windsensor_config", "1", "baud_rate"),
				db.retrieveCellAsInt("buffer_config", "1", "windsensor")
			)
		);

		bool xBee_sending = db.retrieveCellAsInt("xbee_config", "1", "send");
		bool xBee_receiving = db.retrieveCellAsInt("xbee_config", "1", "recieve");
		bool xBee_sendLogs = db.retrieveCellAsInt("xbee_config", "1", "send_logs");
		double xBee_loopTime = stod(db.retrieveCell("xbee_config", "1", "loop_time"));

		xbee_handle = new xBeeSync(&externalCommand, &systemstate, &db, xBee_sendLogs, xBee_sending, xBee_receiving,xBee_loopTime);

		if(xbee_handle->init())
		{
			// Start xBeeSync thread
			std::unique_ptr<ThreadRAII> xbee_sync_thread;

			if (xBee_sending || xBee_receiving)
			{
				xbee_sync_thread = std::unique_ptr<ThreadRAII>(new ThreadRAII(std::thread(threadXBeeSyncRun), ThreadRAII::DtorAction::detach));
			}
		}

		// I2CController thread
//		bool mockArduino = db.retrieveCellAsInt("mock","1","analog_arduino");
//    	bool mockCompass = db.retrieveCellAsInt("mock","1","compass");
//		int  headningBufferSize = db.retrieveCellAsInt("buffer_config", "1", "compass");
//		double i2cLoopTime = stod(db.retrieveCell("i2c_config", "1", "loop_time"));
//
//		if(mockArduino) { Logger::warning("Using mock Arduino"); }
//		if(mockArduino) { Logger::warning("Using mock compass"); }
//
//
//		// Start i2cController thread


		// Start GPSupdater thread
		ThreadRAII gps_reader_thread(
			std::thread(threadGPSupdate),
			ThreadRAII::DtorAction::detach
		);

		// Start httpsync thread
		httpsync_thread = std::unique_ptr<ThreadRAII>(new ThreadRAII(
			std::thread(threadHTTPSyncRun),
			ThreadRAII::DtorAction::detach
		) );

		// Start windsensor thread
		windsensor_thread = std::unique_ptr<ThreadRAII>(new ThreadRAII(
			std::thread(threadWindsensor),
			ThreadRAII::DtorAction::detach
		) );

		sr_handle.run();
	}
	catch (const char * e) {
		printf("ERROR[%s]\n\n",e);
		return 1;
	}


	delete xbee_handle;

	printf("-Finished.\n");
	return 0;
}*/
