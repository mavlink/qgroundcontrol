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
    uint32_t timeMilliseconds; /* Since UAV boot */
    int32_t altitudeMillimetersMSL;
    float absolutePressureMillibars; /* mB == hPa */
    float temperatureKelvin;
    float relativeHumidity;
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
    float groundSpeedMetersPerSecond;

    /* Converted (With possible negligable loss) to more human understandable units */
    double timeSeconds; /* Since UAV boot */
    double altitudeMetersMSL; /* meters */
    float temperatureCelsius;
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

    BalancedDataRecord data;

    uint32_t timeOffset = 0; /* difference between this PC and drone time in milliseconds */
    uint32_t timeOffsetAlt = 0; /* same thing for non-cass messages */
    uint32_t lastUpdate = UINT32_MAX; /* time since last BalancedDataRecord creation */
    uint32_t balancedDataFrequency = 1000; /* min milliseconds between BalancedDataRecord creation */

    /* ring buffers, one for each type of message. Should just store the relevent contents of the message, but this is bloated enough it doesn't matter */
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

    size_t cass0Count = 0;
    size_t cass1Count = 0;
    size_t cass2Count = 0;
    size_t cass3Count = 0;
    size_t count0 = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    size_t count4 = 0;

    mavlink_cass_sensor_raw_t cass0Buf[bufSize];
    float cass0Avg = .0f;    
    /* more buffers here, of various types, with averages */

public:
    void update(const mavlink_message_t* m, Fact* fact);    
};

#endif // DATABALANCER_H
