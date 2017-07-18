#include "Utility.h"
#include <algorithm>
#include <cmath>


int Utility::combineBytes(uint8_t MSB, uint8_t LSB)
{
	int combined = 0;

	combined = MSB;
	combined = (MSB << 8) + LSB;

	return combined;
}

int Utility::combineBytesSigned(uint8_t MSB, uint8_t LSB)
{
	int16_t combined = 0;

	combined = MSB;
	combined = (MSB << 8) + LSB;

	return int(combined);
}

bool Utility::IsOutOfRange (float f) {
	return (f < 90 || f > 270);
}

std::vector<float> Utility::fixAngles(std::vector<float> v){
	unsigned int counter =  count_if(v.begin(), v.end(), Utility::IsOutOfRange);
	if (counter > (v.size()/2)) {
		std::vector<float> newV(v);
		for (float& f : newV) {
			if (f > 180 && f < 360) {
				f -= 360;
			}
		}
		return newV;
	}
	return v;
}

float Utility::getMedianValue(std::vector<float> v) {
	if(v.empty()) {
		return 0;
	}
	std::vector<float> fixedV = Utility::fixAngles(v);
	sort(fixedV.begin(), fixedV.begin() + fixedV.size());
	unsigned int middle = (int) fixedV.size()/2;
	float middleValue = 0;
	if (fixedV.size() % 2 == 1) {
		middleValue = fixedV.at(middle);
	}
	else {
		if (fixedV.size() > middle) {
			middleValue = (fixedV.at(middle-1) + fixedV.at(middle)) / 2;
		}
		else {
			middleValue = fixedV.at(middle);
		}
	}
	if(middleValue < 0 ){
		middleValue+=360;
	}
	return middleValue;
}

float Utility::mean(std::vector<float> values)
{
	if (values.size() < 1) return 0;

	float sum = 0;

	for(std::vector<float>::iterator it = values.begin();
		it != values.end(); ++it)
	{
		sum += *it;
	}

	return sum / values.size();
}

float Utility::mapInterval(float val, float fromMin, float fromMax, float toMin, float toMax) {
  return (val - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
}

/*
 * uses formula for calculating mean of angles
 * https://en.wikipedia.org/wiki/Mean_of_circular_quantities
 */
float Utility::meanOfAngles(std::vector<float> anglesInDegrees)
{
	if (anglesInDegrees.size() < 1) return 0;

	std::vector<float> xx, yy;
	float x, y;

	// convert all angles to cartesian coordinates
	for(std::vector<float>::iterator it = anglesInDegrees.begin();
		it != anglesInDegrees.end(); ++it)
	{
		polarToCartesian(*it, x, y);
		xx.push_back(x);
		yy.push_back(y);
	}

	// use formula
	float meanAngleRadians = atan2(mean(yy), mean(xx));
	// atan2 produces results in the range (−π, π],
	// which can be mapped to [0, 2π) by adding 2π to negative results
	if (meanAngleRadians < 0) meanAngleRadians += 2*M_PI;

	return meanAngleRadians * 180/M_PI;
}

int Utility::sgn(double value)
{
	if(value == 0) return 0;
	if(value < 0) return -1;
	if(value > 0) return 1;

	return 0;
}

void Utility::polarToCartesian(float degrees, float& x, float& y)
{
	x = cos(degrees * M_PI/180);
	y = sin(degrees * M_PI/180);
}

bool Utility::isAngleInSector(double angle, double sectorAngle1, double sectorAngle2)
{
	double start = 0;
	double end = limitAngleRange(sectorAngle2 - sectorAngle1);
	double toCheck = limitAngleRange(angle - sectorAngle1);

	bool angleIsInSector = false;
	if (toCheck >= start && toCheck <= end)
		angleIsInSector = true;

	return angleIsInSector;
}


double Utility::angleDifference(double angle1, double angle2)
{
	const double fullRevolution = 360;

	double diff = std::abs(limitAngleRange(angle1) - limitAngleRange(angle2));
	if (diff > fullRevolution/2) diff = fullRevolution - diff;

	return diff;
}


double Utility::limitAngleRange(double angle)
{
	const double fullRevolution = 360;
	const double minAngle = 0;

	while (angle < minAngle)
		angle += fullRevolution;

	while (angle >= minAngle + fullRevolution)
		angle -= fullRevolution;

	return angle;
}

double Utility::limitRadianAngleRange(double angle)
{
	const double fullRevolution = 2 * M_PI;
	const double minAngle = 0;

	while (angle < minAngle)
		angle += fullRevolution;

	while (angle >= minAngle + fullRevolution)
		angle -= fullRevolution;

	return angle;
}


double Utility::degreeToRadian(double degrees)
{
	return degrees * M_PI / 180;
}


double Utility::radianToDegree(double radians)
{
	return radians / M_PI * 180;
}

int16_t Utility::headingDifference(uint16_t h1, uint16_t h2)
{
	int16_t diff = h2 - h1;
	int16_t absDiff = abs( diff );

	if (absDiff <= 180)
	{
		return absDiff == 180 ? absDiff : diff;
	}

	else if (h2 > h1)
	{
		return absDiff - 360;
	}

	else
	{
		return 360 - absDiff;
	}
}

int16_t Utility::signedHeadingDifference(uint16_t h1, uint16_t h2)
{
	return (h2 - h1 + 540) % 360 - 180;
}

uint16_t Utility::wrapAngle( int16_t angle)
{
	while ( angle < 0 || angle >= 360 )
    {
        if ( angle < 0 )
        {
            angle += 360;
        }
        else
        {
            angle -= 360;
        }
    }

    return angle;
}

int Utility::addDeclinationToHeading(int heading, int declination) {
	return static_cast<int> (Utility::limitAngleRange(heading + declination) + 0.5);
}

double Utility::directionAdjustedSpeed(double gpsHeading,double compassHeading,double gpsSpeed) {
	double speed = 0;

		if (Utility::angleDifference(gpsHeading,compassHeading) < 90)
		{
			speed = gpsSpeed;
		}

		return speed;
}

double Utility::calculateSignedDistanceToLine(const double nextLon, const double nextLat, const double prevLon, const double prevLat, 
					const double gpsLon, const double gpsLat)
{
    int earthRadius = 6371000;
    
    std::array<double, 3> prevWPCoord = //a
     {  earthRadius * cos(degreeToRadian(prevLat)) * cos(degreeToRadian(prevLon)),
        earthRadius * cos(degreeToRadian(prevLat)) * sin(degreeToRadian(prevLon)),
        earthRadius * sin(degreeToRadian(prevLat))};
    std::array<double, 3> nextWPCoord = //b
     {  earthRadius * cos(degreeToRadian(nextLat)) * cos(degreeToRadian(nextLon)),
        earthRadius * cos(degreeToRadian(nextLat)) * sin(degreeToRadian(nextLon)),
        earthRadius * sin(degreeToRadian(nextLat))};
        std::array<double, 3> boatCoord = //m
     {  earthRadius * cos(degreeToRadian(gpsLat)) * cos(degreeToRadian(gpsLon)),
        earthRadius * cos(degreeToRadian(gpsLat)) * sin(degreeToRadian(gpsLon)),
        earthRadius * sin(degreeToRadian(gpsLat))};
    
    std::array<double, 3> oab = //vector normal to plane
    {   (prevWPCoord[1]*nextWPCoord[2] - prevWPCoord[2]*nextWPCoord[1]),       //Vector product: A^B divided by norm ||a^b||     a^b / ||a^b||
        (prevWPCoord[2]*nextWPCoord[0] - prevWPCoord[0]*nextWPCoord[2]),
        (prevWPCoord[0]*nextWPCoord[1] - prevWPCoord[1]*nextWPCoord[0])};

    double normOAB =  sqrt(pow(oab[0],2)+ pow(oab[1],2) + pow(oab[2],2));

    oab[0] = oab[0]/normOAB;
    oab[1] = oab[1]/normOAB;
    oab[2] = oab[2]/normOAB;

    double signedDistance = boatCoord[0]*oab[0] + boatCoord[1]*oab[1] + boatCoord[2]*oab[2];

    return signedDistance;
}

double Utility::calculateWaypointsOrthogonalLine(const double nextLon, const double nextLat, const double prevLon, const double prevLat, 
					const double gpsLon, const double gpsLat)
{    /* Check to see if boat has passed the orthogonal to the line
     * otherwise the boat will continue to follow old line if it passed the waypoint without entering the radius
     */
    int earthRadius = 6371000;
    
    std::array<double, 3> prevWPCoord = //a
     {  earthRadius * cos(degreeToRadian(prevLat)) * cos(degreeToRadian(prevLon)),
        earthRadius * cos(degreeToRadian(prevLat)) * sin(degreeToRadian(prevLon)),
        earthRadius * sin(degreeToRadian(prevLat))};
    std::array<double, 3> nextWPCoord = //b
     {  earthRadius * cos(degreeToRadian(nextLat)) * cos(degreeToRadian(nextLon)),
        earthRadius * cos(degreeToRadian(nextLat)) * sin(degreeToRadian(nextLon)),
        earthRadius * sin(degreeToRadian(nextLat))};
        std::array<double, 3> boatCoord = //m
     {  earthRadius * cos(degreeToRadian(gpsLat)) * cos(degreeToRadian(gpsLon)),
        earthRadius * cos(degreeToRadian(gpsLat)) * sin(degreeToRadian(gpsLon)),
        earthRadius * sin(degreeToRadian(gpsLat))};
    
    std::array<double, 3> oab = //vector normal to plane
    {   (prevWPCoord[1]*nextWPCoord[2] - prevWPCoord[2]*nextWPCoord[1]),       //Vector product: A^B divided by norm ||a^b||     a^b / ||a^b||
        (prevWPCoord[2]*nextWPCoord[0] - prevWPCoord[0]*nextWPCoord[2]),
        (prevWPCoord[0]*nextWPCoord[1] - prevWPCoord[1]*nextWPCoord[0])};

    double normOAB =  sqrt(pow(oab[0],2)+ pow(oab[1],2) + pow(oab[2],2));

    oab[0] = oab[0]/normOAB;
    oab[1] = oab[1]/normOAB;
    oab[2] = oab[2]/normOAB;

    //compute if boat is after waypointModel
    std::array<double, 3> orthogonal_to_AB_from_B = //C the point such as  BC is orthogonal to AB
    {  nextWPCoord[0]+oab[0],
       nextWPCoord[1]+oab[1],
       nextWPCoord[2]+oab[2]
    };

    std::array<double, 3> obc = //vector normal to plane
    {   (orthogonal_to_AB_from_B[1]*nextWPCoord[2] - orthogonal_to_AB_from_B[2]*nextWPCoord[1]) ,       //Vector product: C^B divided by norm ||c^b||     c^b / ||c^b||
        (orthogonal_to_AB_from_B[2]*nextWPCoord[0] - orthogonal_to_AB_from_B[0]*nextWPCoord[2]) ,
        (orthogonal_to_AB_from_B[0]*nextWPCoord[1] - orthogonal_to_AB_from_B[1]*nextWPCoord[0])};

    double normOBC =  sqrt(pow(obc[0],2)+ pow(obc[1],2) + pow(obc[2],2));

	double orthogonalLine;
    //float temp = boatCoord[0]*obc[0] + boatCoord[1]*obc[1] + boatCoord[2]*obc[2];
    orthogonalLine = boatCoord[0]*obc[0]/normOBC + boatCoord[1]*obc[1]/normOBC + boatCoord[2]*obc[2]/normOBC;

    return orthogonalLine;
}

/*
 * uses formula for calculating true Wind Direction
 * https://en.wikipedia.org/wiki/Apparent_wind
 */
double Utility::calculateTrueWindDirection(const int windsensorDir, const int windsensorSpeed, const double gpsSpeed, const double heading){

	//double knots = 1.94384;
	double apparentWindSpeed = windsensorSpeed; //* knots; // Converting m/s to knots
	double apparentWindAngle = windsensorDir;
	double boatSpeed = gpsSpeed;

	if (apparentWindAngle < 0.001 ){
		apparentWindAngle = 0.001;
	} else if ( apparentWindAngle > 359.999 ){
		apparentWindAngle = 359.999 ;
	}

	if(apparentWindSpeed < 0.001){
		return heading;
	}

	double trueWindSpeed = sqrt((apparentWindSpeed * apparentWindSpeed) + (boatSpeed * boatSpeed)
						 - (2 * boatSpeed * apparentWindSpeed * cos(apparentWindAngle/180*M_PI)));

	double alpha = acos((apparentWindSpeed * cos(apparentWindAngle/180*M_PI) - boatSpeed)
					/ trueWindSpeed)* 180/M_PI;

	double twd = 0;
	if (apparentWindAngle > 180){
		twd = Utility::limitAngleRange(heading - alpha);
	} else {
		twd = Utility::limitAngleRange(heading + alpha);
	}

	return twd;
}

double Utility::calculateTrueWindSpeed(int windsensorDir, int windsensorSpeed, double gpsSpeed, double heading)
{
	//double knots = 1.94384;
	double apparentWindSpeed = windsensorSpeed; //* knots; // Converting m/s to knots
	double apparentWindAngle = degreeToRadian( windsensorDir );
	//double boatSpeed = gpsSpeed;

	if (apparentWindAngle < 0.001 ){
		apparentWindAngle = 0.001;
	} else if ( apparentWindAngle > 359.999 ){
		apparentWindAngle = 359.999 ;
	}

	if(apparentWindSpeed < 0.001){
		return heading;
	}

	double u = gpsSpeed * sin( heading ) - windsensorSpeed * sin (apparentWindAngle);
	double v = gpsSpeed * cos( heading ) - windsensorSpeed * cos (apparentWindAngle);

	return atan( u / v );


	//double trueWindSpeed = sqrt((apparentWindSpeed * apparentWindSpeed) + (boatSpeed * boatSpeed)
					//	 - (2 * boatSpeed * apparentWindSpeed * cos(apparentWindAngle/180*M_PI)));

	//return trueWindSpeed;
}

double Utility::getTrueWindDirection(int windsensorDir, int windsensorSpeed, double gpsSpeed, int compassHeading, 
			std::vector<float> &twdBuffer, const unsigned int twdBufferMaxSize)
{
	static unsigned int trueWindIndex = 0;
	double twd = calculateTrueWindDirection(windsensorDir, windsensorSpeed, gpsSpeed, compassHeading);

	if(twdBuffer.size() < twdBufferMaxSize)
	{
		twdBuffer.push_back(twd);
	}
	else
	{
		if(trueWindIndex >= twdBufferMaxSize)
		{
			trueWindIndex = 0;
		}

		twdBuffer[trueWindIndex] = twd;
		trueWindIndex++;
	}

	return meanOfAngles(twdBuffer);
}

void Utility::calculateApparentWind(const int windsensorDir, const int windsensorSpeed, const double gpsSpeed, const double heading,
	           const double trueWindDirection,double &apparentWindSpeed, double &apparentWindDirection)
{ //referenced from "Modeling, control and state-estimation for an autonomous sailboat" by Jon Melin
  /* the trueWindDirection is the origin of the wind but the computation suppose its the direction so we change it during the function*/

  double trueWindDirection_ = trueWindDirection+M_PI;
	std::array<double, 2> wcaw = { calculateTrueWindSpeed(windsensorDir, windsensorSpeed, gpsSpeed, heading) * cos(trueWindDirection_ - heading) - gpsSpeed, //wcaw[0]
									calculateTrueWindSpeed(windsensorDir, windsensorSpeed, gpsSpeed, heading) * sin(trueWindDirection_ - heading)}; //wcaw[1] Cartesian Apparent Wind

	apparentWindSpeed = sqrt(pow(wcaw[0], 2) + pow(wcaw[1], 2));
	apparentWindDirection = -atan2(wcaw[0], wcaw[1])*180/M_PI;
}

double Utility::getApparentWindSpeed(const int windsensorDir, const int windsensorSpeed, const double gpsSpeed, const double heading, const double trueWindDirection)
{
	double apparentWindSpeed,apparentWindDirection;
	calculateApparentWind(windsensorDir, windsensorSpeed, gpsSpeed, heading, trueWindDirection,apparentWindSpeed,apparentWindDirection);
	return apparentWindSpeed;
}

double Utility::getApparentWindDirection(const int windsensorDir, const int windsensorSpeed, const double gpsSpeed, const double heading, const double trueWindDirection)
{
	double apparentWindSpeed,apparentWindDirection;
	calculateApparentWind(windsensorDir, windsensorSpeed, gpsSpeed, heading, trueWindDirection,apparentWindSpeed,apparentWindDirection);
	return apparentWindDirection;
}

void Utility::sphericalCoordinateSystem( const double lat, const double lon, double& x, double& y)
{
	static const double EARTH_RADIUS = 6371.0;

	double latR = lat * M_PI / 180;
	double lonR = lon * M_PI / 180;

	x = cos(latR) * cos(lonR) * EARTH_RADIUS;
	y = cos(latR) * sin(lonR) * EARTH_RADIUS;
}

void Utility::calculateVelocity( const uint16_t course, const double speed, double& vX, double& vY )
{
	vX = speed * cos(course * (M_PI / 180));
	vY = speed * sin(course * (M_PI / 180));
}