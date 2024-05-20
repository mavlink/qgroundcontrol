#ifndef DATABALANCER_H
#define DATABALANCER_H

#include "ardupilotmega/mavlink.h"
#include "ardupilotmega/mavlink_msg_cass_sensor_raw.h"
#include "mavlink_types.h"
#include "FactGroup.h"

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
    uint16_t headingGlobalCentidegrees; /* Relative to true north */
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

/* The AltitudeLevelMessage differs from the IMetData primarily in that the IMetData curates data on the basis of A. Having at least one of everything,
 * and B. a specified frequency (balancedDataFrequency). While the AltitudeLevelMessage curates data based on altitude (binSize). AltitudeLevelMessage is also less
 * verbose, and only has the 17 members actually needed for the file output and GUI display, while IMetData has everything that could conceivably be useful.
*/
typedef struct {
    float asl;
    double time;
    float pressure;
    float airTemp;
    float relHum;
    float windSpeed;
    float windDirection;
    double latitude;
    double longitude;
    float roll;
    float rollRate;
    float pitch;
    float pitchRate;
    float yaw;
    float yawRate;
    float ascentRate;
    float speedOverGround;
} AltitudeLevelMessage;

class DataBalancer{
public:
    enum ALMStatus {
        SUCCESS = 0,
        DATA_NOT_INITIALIZED = -1,
        NOT_ASCENDING = -2,
        ALTITUDE_CHANGE_TOO_SMALL = -3
    };
private:
    IMetData data;
    bool dataInit = false;
    AltitudeLevelMessage alm;

    uint64_t UAVBootMilliseconds = 0; /* UAV boot time from Unix reference frame */    
    uint64_t lastUpdate = UINT64_MAX; /* time since last IMetDataRaw creation */
    uint32_t balancedDataFrequency = 50; /* min milliseconds between BalancedDataRecord creation */

    /* IMetData helpers */
    size_t cassTemp0C = 0;
    float cassTemp0A = .0f;
    size_t cassTemp1C = 0;
    float cassTemp1A = .0f;
    size_t cassTemp2C = 0;
    float cassTemp2A = .0f;
    size_t cassRH0C = 0;
    float cassRH0A = .0f;
    size_t cassRH1C = 0;
    float cassRH1A = .0f;
    size_t cassRH2C = 0;
    float cassRH2A = .0f;
    size_t altMmC = 0;
    int32_t altMmA = 0;
    size_t absolutePressureMillibarsC = 0;
    float absolutePressureMillibarsA = .0f;
    size_t rollRadiansC = 0;
    float rollRadiansA = .0f;
    size_t pitchRadiansC = 0;
    float pitchRadiansA = .0f;
    size_t yawRadiansC = 0;
    float yawRadiansA = .0f;
    size_t headingGlobalCentidegreesC = 0;
    uint16_t headingGlobalCentidegreesA = 0;
    size_t rollRateRadiansPerSecondC = 0;
    float rollRateRadiansPerSecondA = .0f;
    size_t pitchRateRadiansPerSecondC = 0;
    float pitchRateRadiansPerSecondA = .0f;
    size_t yawRateRadiansPerSecondC = 0;
    float yawRateRadiansPerSecondA = .0f;
    size_t latitudeDegreesE7C = 0;
    int32_t latitudeDegreesE7A = 0;
    size_t longitudeDegreesE7C = 0;
    int32_t longitudeDegreesE7A = 0;
    size_t zVelocityC = 0;
    float zVelocityA = .0f;
    size_t xVelocityC = 0;
    float xVelocityA = .0f;
    size_t yVelocityC = 0;
    float yVelocityA = .0f;

    /* Altitude Level Message helpers */
    int ALMInit = 0;
    float lowBinAlt = .0f;
    float lowBinTime = .0f;
    float highBinTime = .0f;
    float binSize = 5.0f;
    size_t aslC = 0;
    float aslA = .0f;
    size_t timeC = 0;
    double timeA = .0;
    size_t pressureC = 0;
    float pressureA = .0f;
    size_t airTempC = 0;
    float airTempA = .0f;
    size_t relHumC = 0;
    float relHumA = .0f;
    size_t windSpeedC = 0;
    float windSpeedA = .0f;
    size_t windDirectionC = 0;
    float windDirectionA = .0f;
    size_t latitudeC = 0;
    float latitudeA = .0f;
    size_t longitudeC = 0;
    float longitudeA = .0f;
    size_t rollC = 0;
    float rollA = .0f;
    size_t rollRateC = 0;
    float rollRateA = .0f;
    size_t pitchC = 0;
    float pitchA = .0f;
    size_t pitchRateC = 0;
    float pitchRateA = .0f;
    size_t yawC = 0;
    float yawA = .0f;
    size_t yawRateC = 0;
    float yawRateA = .0f;
    size_t ascentRateC = 0;
    float ascentRateA = .0f;
    size_t speedOverGroundC = 0;
    float speedOverGroundA = .0f;

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

    int updateALM();
    void onALMUpdate(Fact* asl, Fact* time, Fact* pressure, Fact* airTemp, Fact* relHum, Fact* windSpeed, Fact* windDirection, Fact* latitude, Fact* longitude,
                     Fact* roll, Fact* rollRate, Fact* pitch, Fact* pitchRate, Fact* yaw, Fact* yawRate, Fact* ascentRate, Fact* speedOverGround,
                     Fact* update);
private:
    static void calcWindProps(IMetData* d);
    static void calcGroundSpeed(IMetData* d);
};

#endif // DATABALANCER_H
