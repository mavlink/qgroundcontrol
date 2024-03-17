#include "DataBalancer.h"
#include <chrono>

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

void DataBalancer::update(const mavlink_message_t* m, Fact* tempFact){
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
            cassTemp0Avg = ((cassTemp0Avg * cassTemp0Count) + s.values[0]) / (cassTemp0Count++ + 1);
            cassTemp1Avg = ((cassTemp1Avg * cassTemp1Count) + s.values[1]) / (cassTemp1Count++ + 1);
            cassTemp2Avg = ((cassTemp2Avg * cassTemp2Count) + s.values[2]) / (cassTemp2Count++ + 1);
            break;
        }
        case 1:{ /* iMet RH */            
            cassRH0Avg = ((cassRH0Avg * cassRH0Count) + s.values[0]) / (cassRH0Count++ + 1);
            cassRH1Avg = ((cassRH1Avg * cassRH1Count) + s.values[1]) / (cassRH1Count++ + 1);
            cassRH2Avg = ((cassRH2Avg * cassRH2Count) + s.values[2]) / (cassRH2Count++ + 1);
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
        mavlink_global_position_int_t s;
        mavlink_msg_global_position_int_decode(m, &s);
        zVelocityAvg = ((zVelocityAvg * zVelocityCount) + s.vz) / (zVelocityCount++ + 1);
        xVelocityAvg = ((xVelocityAvg * xVelocityCount) + s.vx) / (xVelocityCount++ + 1);
        yVelocityAvg = ((yVelocityAvg * yVelocityCount) + s.vy) / (yVelocityCount++ + 1);
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:{
        mavlink_global_position_int_t s;
        mavlink_msg_global_position_int_decode(m, &s);
        altMmAvg = (((int64_t)altMmAvg * altMmCount) + s.alt) / (altMmCount++ + 1);
        latitudeAvg = (((int64_t)latitudeAvg * latitudeCount) + s.lat) / (latitudeCount++ + 1);
        longitudeAvg = (((int64_t)longitudeAvg * longitudeCount) + s.lon) / (longitudeCount++ + 1);
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE2:{
        mavlink_scaled_pressure2_t s;
        mavlink_msg_scaled_pressure2_decode(m, &s);
        pressureAvg = ((pressureAvg * pressureCount) + s.press_abs) / (pressureCount++ + 1);
        break;
    }
    case MAVLINK_MSG_ID_ATTITUDE:{
        mavlink_attitude_t s;
        mavlink_msg_attitude_decode(m, &s);
        rollAvg = ((rollAvg * rollCount) + s.roll) / (rollCount++ + 1);
        pitchAvg = ((pitchAvg * pitchCount) + s.pitch) / (pitchCount++ + 1);
        yawAvg = ((yawAvg * yawCount) + s.yaw) / (yawCount++ + 1);
        rollRateAvg = ((rollRateAvg * rollRateCount) + s.rollspeed) / (rollRateCount + 1);
        rollRateCount++;
        pitchRateAvg = ((pitchRateAvg * pitchRateCount) + s.pitchspeed) / (pitchRateCount++ + 1);
        yawRateAvg = ((yawRateAvg * yawRateCount) + s.yawspeed) / (yawRateCount++ + 1);
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
          latitudeCount > 0 && longitudeCount > 0 && zVelocityCount > 0 && xVelocityCount > 0 && yVelocityCount > 0)) return;

    /* Too soon... */
    if ((currentTime - lastUpdate) < balancedDataFrequency) return;
    
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
    data.relativeHumidity = (cassTemp0Avg + cassTemp1Avg + cassTemp2Avg) / 3;
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
    data.latitudeDegrees = (double)latitudeAvg / 10e7;
    data.longitudeDegrees = (double)longitudeAvg / 10e7;
    data.zVelocityMetersPerSecondInverted = zVelocityAvg;
    data.xVelocityMetersPerSecond = xVelocityAvg;
    data.yVelocityMetersPerSecond = yVelocityAvg;
    data.zVelocityMetersPerSecond = -data.zVelocityMetersPerSecondInverted;
    calcGroundSpeed(&data);
    data.ascending = data.heartBeatCustomMode == 3 && data.zVelocityMetersPerSecond > 2.5f;

    /* Update facts */
    tempFact->setRawValue(cassTemp0Avg);

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
