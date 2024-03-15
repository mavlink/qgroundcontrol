#ifndef DATABALANCER_H
#define DATABALANCER_H

#include "ardupilotmega/mavlink.h"
#include "ardupilotmega/mavlink_msg_cass_sensor_raw.h"
#include "mavlink_types.h"
#include "FactGroup.h"

typedef struct {
    uint32_t time; /* drone's frame of reference */
    /* cass sensor type 0 (iMet temp) */
    float iMetTemp;
    /* rest of variables... */
} BalancedDataRecord;

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
    // up to here
    int32_t latitudeDegreesE7;
    int32_t longitudeDegreesE7;
    float rollRadians;
    float pitchRadians;
    float yawRadians;
    float rollRateRadiansPerSecond;
    float pitchRateRadiansPerSecond;
    float yawRateRadiansPerSecond;
    float zVelocityMetersPerSecondInverted; /* mavlink uses flipped z axis */
    float groundSpeedMetersPerSecond;

    /* Converted (With possible negligable loss) to more human understandable units */
    double timeUAVSeconds;
    double timeUnixSeconds;
    double timeUAVBootSeconds;
    double altitudeMetersMSL;
    float temperatureCelsius; /* average of msg 227 type 0 elements 0, 1, and 2 */
    // up to this line
    float latitudeDegrees;
    float longitudeDegrees;
    float rollDegrees;
    float pitchDegrees;
    float yawDegrees;
    float rollRateDegreesPerSecond;
    float pitchRateDegreesPerSecond;
    float yawRateDegreesPerSecond;
    float zVelocityMetersPerSecond;
} IMetData;

class DataBalancer{
    IMetData data;

    uint64_t UAVBootMilliseconds = 0; /* UAV boot time from Unix reference frame */
    uint64_t timeOffsetAlt = 0; /* same thing for non-cass messages */
    uint64_t lastUpdate = UINT64_MAX; /* time since last IMetDataRaw creation */
    uint32_t balancedDataFrequency = 1000; /* min milliseconds between BalancedDataRecord creation */


    /* Deprecated - we don't actually need these */
    /* ring buffers, one for each type of message. Should just store the relevent contents of the message, but this is bloated enough it doesn't matter */
    /*
    size_t static constexpr bufSize = 8;

    size_t cass0Head = 0;
    size_t cass1Head = 0;
    size_t cass2Head = 0;
    size_t cass3Head = 0;
    size_t head0 = 0;
    size_t head1 = 0;
    size_t head2 = 0;
    size_t head3 = 0;
    size_t head4 = 0;

    mavlink_cass_sensor_raw_t cass0Buf[bufSize];
    */

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

    size_t cass1Count = 0;
    size_t cass2Count = 0;
    size_t cass3Count = 0;
    size_t count0 = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    size_t count4 = 0;

public:
    void update(const mavlink_message_t* m, Fact* fact);
    static void calcWindProps(IMetData* d);
};

#endif // DATABALANCER_H
