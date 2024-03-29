#ifndef DATABALANCER_H
#define DATABALANCER_H

#include "ardupilotmega/mavlink.h"
#include "ardupilotmega/mavlink_msg_cass_sensor_raw.h"
#include "mavlink_types.h"
#include "FactGroup.h"

/*  Deprecated */
// typedef struct {
//     uint32_t time; /* drone's frame of reference */
//     /* cass sensor type 0 (iMet temp) */
//     float iMetTemp;
//     /* rest of variables... */
// } BalancedDataRecord;

typedef struct {
    /* Original data, either as calculated or directly from the mavlink messages */
    uint32_t timeUAVMilliseconds; /* Since UAV boot */
    uint64_t timeUnixMilliseconds;
    uint64_t timeUAVBootMilliseconds; /* UAV boot time from Unix reference frame */
    int32_t altitudeMillimetersMSL;
    float absolutePressureMillibars; /* mB == hPa */
    float temperature0Kelvin; /* msg 227 type 0 element 0 */
    float temperature1Kelvin; /* msg 227 type 0 element 1 */
    float temperature2Kelvin; /* msg 227 type 0 element 2 */
    float relativeHumidity; /* average of msg 227 type 1 elements 0, 1, and 2 */
    float relativeHumidity0; /* msg 227 type 1 element 0 */
    float relativeHumidity1; /* msg 227 type 1 element 1 */
    float relativeHumidity2; /* msg 227 type 1 element 3 */    
    float windSpeedMetersPerSecond;
    float windBearingDegrees; /* True or magnetic north? */    
    int32_t latitudeDegreesE7;
    int32_t longitudeDegreesE7;    
    float rollRadians;
    float pitchRadians;
    float yawRadians;
    float rollRateRadiansPerSecond;
    float pitchRateRadiansPerSecond;
    float yawRateRadiansPerSecond;    
    float zVelocityMetersPerSecondInverted; /* mavlink uses flipped z axis */
    float xVelocityMetersPerSecond;
    float yVelocityMetersPerSecond;
    float groundSpeedMetersPerSecond;
    uint32_t heartBeatCustomMode;    

    /* Converted (With possible negligable loss) to more human understandable units or types. */
    double timeUAVSeconds;
    double timeUnixSeconds;
    double timeUAVBootSeconds;
    double altitudeMetersMSL;
    float temperatureCelsius; /* average of msg 227 type 0 elements 0, 1, and 2 */    
    double latitudeDegrees;
    double longitudeDegrees;
    float rollDegrees;
    float pitchDegrees;
    float yawDegrees;
    float rollRateDegreesPerSecond;
    float pitchRateDegreesPerSecond;
    float yawRateDegreesPerSecond;    
    float zVelocityMetersPerSecond;
    bool ascending;
    bool lastState;
    size_t ascents;
} IMetData;

class DataBalancer{
    IMetData data;
    bool dataInit = false;

    uint64_t UAVBootMilliseconds = 0; /* UAV boot time from Unix reference frame */    
    uint64_t lastUpdate = UINT64_MAX; /* time since last IMetDataRaw creation */
    uint32_t balancedDataFrequency = 200; /* min milliseconds between BalancedDataRecord creation */

    size_t cassTemp0Count = 0;
    float cassTemp0Avg = .0f;
    size_t cassTemp1Count = 0;
    float cassTemp1Avg = .0f;
    size_t cassTemp2Count = 0;
    float cassTemp2Avg = .0f;
    size_t cassRH0Count = 0;
    float cassRH0Avg = .0f;
    size_t cassRH1Count = 0;
    float cassRH1Avg = .0f;
    size_t cassRH2Count = 0;
    float cassRH2Avg = .0f;
    size_t altMmCount = 0;
    int32_t altMmAvg = 0;
    size_t pressureCount = 0;
    float pressureAvg = .0f;
    size_t rollCount = 0;
    float rollAvg = .0f;
    size_t pitchCount = 0;
    float pitchAvg = .0f;
    size_t yawCount = 0;
    float yawAvg = .0f;
    size_t rollRateCount = 0;
    float rollRateAvg = .0f;
    size_t pitchRateCount = 0;
    float pitchRateAvg = .0f;
    size_t yawRateCount = 0;
    float yawRateAvg = .0f;
    size_t latitudeCount = 0;
    int32_t latitudeAvg = .0f;
    size_t longitudeCount = 0;
    int32_t longitudeAvg = .0f;
    size_t zVelocityCount = 0;
    float zVelocityAvg = .0f;
    size_t xVelocityCount = 0;
    float xVelocityAvg = .0f;
    size_t yVelocityCount = 0;
    float yVelocityAvg = .0f;
public:
    void update(const mavlink_message_t* m, Fact* timeUAVMilliseconds, Fact* timeUnixMilliseconds, Fact* timeUAVBootMilliseconds, Fact* altitudeMillimetersMSL,
                Fact* absolutePressureMillibars, Fact* temperature0Kelvin, Fact* temperature1Kelvin, Fact* temperature2Kelvin, Fact* relativeHumidity,
                Fact* relativeHumidity0, Fact* relativeHumidity1, Fact* relativeHumidity2, Fact* windSpeedMetersPerSecond, Fact* windBearingDegrees,
                Fact* latitudeDegreesE7, Fact* longitudeDegreesE7, Fact* rollRadians, Fact* pitchRadians, Fact* yawRadians, Fact* rollRateRadiansPerSecond,
                Fact* pitchRateRadiansPerSecond, Fact* yawRateRadiansPerSecond, Fact* zVelocityMetersPerSecondInverted, Fact* xVelocityMetersPerSecond,
                Fact* yVelocityMetersPerSecond, Fact* groundSpeedMetersPerSecond, Fact* heartBeatCustomMode, Fact* ascending, Fact* timeUAVSeconds,
                Fact* timeUnixSeconds, Fact* timeUAVBootSeconds, Fact* altitudeMetersMSL, Fact* temperatureCelsius, Fact* latitudeDegrees, Fact* longitudeDegrees,
                Fact* rollDegrees, Fact* pitchDegrees, Fact* yawDegrees, Fact* rollRateDegreesPerSecond, Fact* pitchRateDegreesPerSecond,
                Fact* yawRateDegreesPerSecond, Fact* zVelocityMetersPerSecond, Fact* lastState, Fact* ascents);
private:
    static void calcWindProps(IMetData* d);
    static void calcGroundSpeed(IMetData* d);
};

#endif // DATABALANCER_H
