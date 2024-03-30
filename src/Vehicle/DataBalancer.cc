#pragma once

#include "DataBalancer.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "VehicleTemperatureFactGroup.h"
#include <chrono>

#define _CRT_SECURE_NO_WARNINGS
#define DEGREES(radians) ((radians) * (180.0 / M_PI))

void DataBalancer::calcWindProps(IMetData* d){
    float croll = cos(d->rollRadians);
    float sroll = sin(d->rollRadians);
    float cpitch = cos(d->pitchRadians);
    float spitch = sin(d->pitchRadians);
    float cyaw = cos(d->yawRadians);
    float syaw = sin(d->yawRadians);
    double R[3][3] = {
        {cpitch * cyaw,                             cpitch * -syaw,                             spitch          },
        {sroll * spitch * cyaw + croll * syaw,      -sroll * spitch * syaw + croll * cyaw,      -sroll * cpitch },
        {-croll * spitch * cyaw + sroll * syaw,     croll * spitch * syaw + sroll * cyaw,       croll * cpitch  }
    };
    d->windBearingDegrees = DEGREES(atan2(R[1][2], R[0][2]));
    d->windSpeedMetersPerSecond = 39.4f * sqrt(tan(acos(R[2][2]))) - 5.71f;
}

void DataBalancer::calcGroundSpeed(IMetData* d){
    d->groundSpeedMetersPerSecond = sqrt((d->xVelocityMetersPerSecond * d->xVelocityMetersPerSecond) + (d->yVelocityMetersPerSecond * d->yVelocityMetersPerSecond));
}

void DataBalancer::update(const mavlink_message_t* m, Fact* timeUAVMilliseconds, Fact* timeUnixMilliseconds, Fact* timeUAVBootMilliseconds, Fact* altitudeMillimetersMSL,
                          Fact* absolutePressureMillibars, Fact* temperature0Kelvin, Fact* temperature1Kelvin, Fact* temperature2Kelvin, Fact* relativeHumidity,
                          Fact* relativeHumidity0, Fact* relativeHumidity1, Fact* relativeHumidity2, Fact* windSpeedMetersPerSecond, Fact* windBearingDegrees,
                          Fact* latitudeDegreesE7, Fact* longitudeDegreesE7, Fact* rollRadians, Fact* pitchRadians, Fact* yawRadians, Fact* rollRateRadiansPerSecond,
                          Fact* pitchRateRadiansPerSecond, Fact* yawRateRadiansPerSecond, Fact* zVelocityMetersPerSecondInverted, Fact* xVelocityMetersPerSecond,
                          Fact* yVelocityMetersPerSecond, Fact* groundSpeedMetersPerSecond, Fact* heartBeatCustomMode, Fact* ascending, Fact* timeUAVSeconds,
                          Fact* timeUnixSeconds, Fact* timeUAVBootSeconds, Fact* altitudeMetersMSL, Fact* temperatureCelsius, Fact* latitudeDegrees, Fact* longitudeDegrees,
                          Fact* rollDegrees, Fact* pitchDegrees, Fact* yawDegrees, Fact* rollRateDegreesPerSecond, Fact* pitchRateDegreesPerSecond,
                          Fact* yawRateDegreesPerSecond, Fact* zVelocityMetersPerSecond, Fact* lastState, Fact* ascents){
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    switch(m->msgid){        
    case MAVLINK_MSG_ID_CASS_SENSOR_RAW:{
        mavlink_cass_sensor_raw_t s;
        mavlink_msg_cass_sensor_raw_decode(m, &s);

        if (UAVBootMilliseconds == 0){
            UAVBootMilliseconds = currentTime - s.time_boot_ms;
        }

        switch(s.app_datatype){
        case 0:{ /* iMet temp */            
            cassTemp0Avg = ((cassTemp0Avg * cassTemp0Count) + s.values[0]) / (int64_t)(cassTemp0Count + 1);
            cassTemp1Avg = ((cassTemp1Avg * cassTemp1Count) + s.values[1]) / (int64_t)(cassTemp1Count + 1);
            cassTemp2Avg = ((cassTemp2Avg * cassTemp2Count) + s.values[2]) / (int64_t)(cassTemp2Count + 1);
            cassTemp0Count++;
            cassTemp1Count++;
            cassTemp2Count++;
            break;
        }
        case 1:{ /* iMet RH */            
            cassRH0Avg = ((cassRH0Avg * cassRH0Count) + s.values[0]) / (int64_t)(cassRH0Count + 1);
            cassRH1Avg = ((cassRH1Avg * cassRH1Count) + s.values[1]) / (int64_t)(cassRH1Count + 1);
            cassRH2Avg = ((cassRH2Avg * cassRH2Count) + s.values[2]) / (int64_t)(cassRH2Count + 1);
            cassRH0Count++;
            cassRH1Count++;
            cassRH2Count++;
            break;
        }
        case 2:{ /* temp from RH */
            break;
        }
        case 3:{ /* wind */
            break;
        }
        }
        break;
    }
    case MAVLINK_MSG_ID_LOCAL_POSITION_NED:{
        mavlink_local_position_ned_t s;
        mavlink_msg_local_position_ned_decode(m, &s);
        xVelocityAvg = ((xVelocityAvg * xVelocityCount) + s.vx) / (int64_t)(xVelocityCount + 1);
        yVelocityAvg = ((yVelocityAvg * yVelocityCount) + s.vy) / (int64_t)(yVelocityCount + 1);
        zVelocityAvg = ((zVelocityAvg * zVelocityCount) + s.vz) / (int64_t)(zVelocityCount + 1);
        xVelocityCount++;
        yVelocityCount++;
        zVelocityCount++;
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:{
        mavlink_global_position_int_t s;
        mavlink_msg_global_position_int_decode(m, &s);
        altMmAvg =     ((int64_t)(altMmAvg     * altMmCount)     + s.alt) / (int64_t)(altMmCount + 1);
        latitudeAvg =  ((int64_t)(latitudeAvg  * latitudeCount)  + s.lat) / (int64_t)(latitudeCount + 1);
        
        //handle averaging near 180th meridian
        int32_t sLonNormalized = s.lon;
        if (longitudeCount > 0 && (longitudeAvg - s.lon) > 180e7) {
            sLonNormalized += 360e7;
        } else if (longitudeCount > 0 && (longitudeAvg - s.lon) < -180e7) {
            sLonNormalized -= 360e7;
        }

        longitudeAvg = ((int64_t)(longitudeAvg * longitudeCount) + sLonNormalized) / (int64_t)(longitudeCount + 1);
        altMmCount++;
        latitudeCount++;
        longitudeCount++;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE2:{
        mavlink_scaled_pressure2_t s;
        mavlink_msg_scaled_pressure2_decode(m, &s);
        pressureAvg = ((pressureAvg * pressureCount) + s.press_abs) / (pressureCount + 1);
        pressureCount++;
        break;
    }
    case MAVLINK_MSG_ID_ATTITUDE:{
        mavlink_attitude_t s;
        mavlink_msg_attitude_decode(m, &s);
        rollAvg =      ((rollAvg      * rollCount)      + s.roll)       / (int64_t)(rollCount + 1);
        pitchAvg =     ((pitchAvg     * pitchCount)     + s.pitch)      / (int64_t)(pitchCount + 1);
        yawAvg =       ((yawAvg       * yawCount)       + s.yaw)        / (int64_t)(yawCount + 1);
        rollRateAvg =  ((rollRateAvg  * rollRateCount)  + s.rollspeed)  / (int64_t)(rollRateCount + 1);        
        pitchRateAvg = ((pitchRateAvg * pitchRateCount) + s.pitchspeed) / (int64_t)(pitchRateCount + 1);
        yawRateAvg =   ((yawRateAvg   * yawRateCount)   + s.yawspeed)   / (int64_t)(yawRateCount + 1);
        rollCount++;
        pitchCount++;
        yawCount++;
        rollRateCount++;
        pitchRateCount++;
        yawRateCount++;
        break;
    }
    case MAVLINK_MSG_ID_HEARTBEAT:{
        mavlink_heartbeat_t s;
        mavlink_msg_heartbeat_decode(m, &s);
        data.heartBeatCustomMode = s.custom_mode;
    }
    }

    /* Some fields not yet ready... */
    if (!(cassTemp0Count > 0 && cassTemp1Count > 0 && cassTemp1Count > 0 && cassRH0Count > 0 && cassRH1Count > 0 && cassRH2Count > 0 && altMmCount > 0 &&
          pressureCount > 0 && rollCount > 0 && pitchCount > 0 && yawCount > 0 && rollRateCount > 0 && pitchRateCount > 0 && yawRateCount > 0 &&
          latitudeCount > 0 && longitudeCount > 0 && zVelocityCount > 0 && xVelocityCount > 0 && yVelocityCount > 0)
    ) {
        return;
    }

    /* Too soon... */
    if ((currentTime - lastUpdate) < balancedDataFrequency) return;
        
    /* Initialization */
    if (!dataInit) {
        data.ascents = 0;
        data.lastState = false;
        data.ascending = false;
        dataInit = true;
    }

    /* Create IMetData */
    data.timeUAVMilliseconds = currentTime - UAVBootMilliseconds;
    data.timeUnixMilliseconds = currentTime;
    data.timeUAVBootMilliseconds = UAVBootMilliseconds;
    data.timeUnixSeconds = (double)data.timeUnixMilliseconds / 1000;
    data.timeUAVSeconds = (double)data.timeUAVMilliseconds / 1000;
    data.timeUAVBootSeconds = (double)data.timeUAVBootMilliseconds / 1000;
    data.altitudeMillimetersMSL = altMmAvg;
    data.altitudeMetersMSL = (double)altMmAvg / 1000;
    data.absolutePressureMillibars = pressureAvg;
    data.temperature0Kelvin = cassTemp0Avg;
    data.temperature1Kelvin = cassTemp1Avg;
    data.temperature2Kelvin = cassTemp2Avg;
    data.temperatureCelsius = ((cassTemp0Avg + cassTemp1Avg + cassTemp2Avg) / 3) - 273.15;
    data.relativeHumidity0 = cassRH0Avg;
    data.relativeHumidity1 = cassRH1Avg;
    data.relativeHumidity2 = cassRH2Avg;
    data.relativeHumidity = (cassRH0Avg + cassRH1Avg + cassRH2Avg) / 3;
    data.rollRadians = rollAvg;
    data.pitchRadians = pitchAvg;
    data.yawRadians = yawAvg;
    data.rollRateRadiansPerSecond = rollRateAvg;
    data.pitchRateRadiansPerSecond = pitchRateAvg;
    data.yawRateRadiansPerSecond = yawRateAvg;
    data.rollDegrees = DEGREES(data.rollRadians);
    data.pitchDegrees = DEGREES(data.pitchRadians);
    data.yawDegrees = DEGREES(data.yawRadians);
    data.rollRateDegreesPerSecond = DEGREES(data.rollRateRadiansPerSecond);
    data.pitchRateDegreesPerSecond = DEGREES(data.pitchRateRadiansPerSecond);
    data.yawRateDegreesPerSecond = DEGREES(data.yawRateRadiansPerSecond);
    calcWindProps(&data);
    data.latitudeDegreesE7 = latitudeAvg;
    data.longitudeDegreesE7 = longitudeAvg;
    data.latitudeDegrees = (double)latitudeAvg / 1e7;
    data.longitudeDegrees = (double)longitudeAvg / 1e7;
    data.zVelocityMetersPerSecondInverted = zVelocityAvg;
    data.xVelocityMetersPerSecond = xVelocityAvg;
    data.yVelocityMetersPerSecond = yVelocityAvg;
    data.zVelocityMetersPerSecond = -data.zVelocityMetersPerSecondInverted;
    calcGroundSpeed(&data);

    /* Ascent detection
     * This does something in non-steady state conditions, and nothing in steady state conditions.
     * If you want to look at the IMetData directly, the data is valid if (dataInit && data.ascending).
    */
    data.ascending = data.heartBeatCustomMode == 3 && data.zVelocityMetersPerSecond > 2.5f;
    if(data.ascending && !data.lastState) {
        data.ascents++;
        /* This is where we hook in for these.
        createFlightFile(ascents)
        updateGUIAscentsValue(ascents)
        */
    }

    if (!data.ascending && data.lastState){
        /* This is where we hook in for this.
        resetDataProcessing()
        */
    }

    data.lastState = data.ascending;

    timeUAVMilliseconds->setRawValue(data.timeUAVMilliseconds);
    timeUnixMilliseconds->setRawValue(data.timeUnixMilliseconds);
    timeUAVBootMilliseconds->setRawValue(data.timeUAVBootMilliseconds);
    altitudeMillimetersMSL->setRawValue(data.altitudeMillimetersMSL);
    absolutePressureMillibars->setRawValue(data.absolutePressureMillibars);
    temperature0Kelvin->setRawValue(data.temperature0Kelvin);
    temperature1Kelvin->setRawValue(data.temperature1Kelvin);
    temperature2Kelvin->setRawValue(data.temperature2Kelvin);
    relativeHumidity->setRawValue(data.relativeHumidity);
    relativeHumidity0->setRawValue(data.relativeHumidity0);
    relativeHumidity1->setRawValue(data.relativeHumidity1);
    relativeHumidity2->setRawValue(data.relativeHumidity2);
    windSpeedMetersPerSecond->setRawValue(data.windSpeedMetersPerSecond);
    windBearingDegrees->setRawValue(data.windBearingDegrees);
    latitudeDegreesE7->setRawValue(data.latitudeDegreesE7);
    longitudeDegreesE7->setRawValue(data.longitudeDegreesE7);
    rollRadians->setRawValue(data.rollRadians);
    pitchRadians->setRawValue(data.pitchRadians);
    yawRadians->setRawValue(data.yawRadians);
    rollRateRadiansPerSecond->setRawValue(data.rollRateRadiansPerSecond);
    pitchRateRadiansPerSecond->setRawValue(data.pitchRateRadiansPerSecond);
    yawRateRadiansPerSecond->setRawValue(data.yawRateRadiansPerSecond);
    zVelocityMetersPerSecondInverted->setRawValue(data.zVelocityMetersPerSecondInverted);
    xVelocityMetersPerSecond->setRawValue(data.xVelocityMetersPerSecond);
    yVelocityMetersPerSecond->setRawValue(data.yVelocityMetersPerSecond);
    groundSpeedMetersPerSecond->setRawValue(data.groundSpeedMetersPerSecond);
    heartBeatCustomMode->setRawValue(data.heartBeatCustomMode);
    ascending->setRawValue(data.ascending);
    timeUAVSeconds->setRawValue(data.timeUAVSeconds);
    timeUnixSeconds->setRawValue(data.timeUnixSeconds);
    timeUAVBootSeconds->setRawValue(data.timeUAVBootSeconds);
    altitudeMetersMSL->setRawValue(data.altitudeMetersMSL);
    temperatureCelsius->setRawValue(data.temperatureCelsius);
    latitudeDegrees->setRawValue(data.latitudeDegrees);
    longitudeDegrees->setRawValue(data.longitudeDegrees);
    rollDegrees->setRawValue(data.rollDegrees);
    pitchDegrees->setRawValue(data.pitchDegrees);
    yawDegrees->setRawValue(data.yawDegrees);
    rollRateDegreesPerSecond->setRawValue(data.rollRateDegreesPerSecond);
    pitchRateDegreesPerSecond->setRawValue(data.pitchRateDegreesPerSecond);
    yawRateDegreesPerSecond->setRawValue(data.yawRateDegreesPerSecond);
    zVelocityMetersPerSecond->setRawValue(data.zVelocityMetersPerSecond);

    /* Reset counters and lastUpdate */
    cassTemp0Count = 0;
    cassTemp1Count = 0;
    cassTemp2Count = 0;
    cassRH0Count = 0;
    cassRH1Count = 0;
    cassRH2Count = 0;
    altMmCount = 0;
    pressureCount = 0;
    rollCount = 0;
    pitchCount = 0;
    yawCount = 0;
    rollRateCount = 0;
    pitchRateCount = 0;
    yawRateCount = 0;
    latitudeCount = 0;
    longitudeCount = 0;
    zVelocityCount = 0;
    xVelocityCount = 0;
    yVelocityCount = 0;
    lastUpdate = currentTime;
}

#undef DEGREES
