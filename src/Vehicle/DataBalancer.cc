#include "DataBalancer.h"
#include <chrono>

#define DEGREES(radians) ((radians) * (180.0 / M_PI))

float windBearingDegrees(IMetData* d){
    float coefA = 39.4f;
    float coefB = -5.71f;
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
    return DEGREES(atan2(R[1][2], R[0][2]));
}

void DataBalancer::update(const mavlink_message_t* m, Fact* tempFact){
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    switch(m->msgid){        
    case MAVLINK_MSG_ID_CASS_SENSOR_RAW:{
        mavlink_cass_sensor_raw_t s;
        mavlink_msg_cass_sensor_raw_decode(m, &s);

        /* if first cass message, calculate the cass boot time. Do the same thing for altTime in the other cases, provided they have a timestamp at all */
        if (UAVBootMilliseconds == 0){
            UAVBootMilliseconds = currentTime - s.time_boot_ms;
        }

        switch(s.app_datatype){
        case 0:{ /* iMet temp */
            /* Deprecated, not using ring buffers any more
            cass0Buf[cass0Head] = s;
            cass0Head = (1 + cass0Head) % bufSize;
            */
            cassTemp0Avg = ((cassTemp0Avg * cassTemp0Count) + s.values[0]) / (cassTemp0Count++ + 1);
            cassTemp1Avg = ((cassTemp1Avg * cassTemp1Count) + s.values[1]) / (cassTemp1Count++ + 1);
            cassTemp2Avg = ((cassTemp2Avg * cassTemp2Count) + s.values[2]) / (cassTemp2Count++ + 1);

            /* This allows for considering only the most recent x number of messages, optional feature
            if (cassTemp0Count < (bufSize - 1)) cassTemp0Count++;
            */
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
    case 32:        
        break;
    case 33:{
        mavlink_global_position_int_t s;
        mavlink_msg_global_position_int_decode(m, &s);
        altMmAvg = (((int64_t)altMmAvg * altMmCount) + s.alt) / (altMmCount++ + 1);
        break;
    }
    case 137:{
        mavlink_scaled_pressure2_t s;
        mavlink_msg_scaled_pressure2_decode(m, &s);
        pressureAvg = ((pressureAvg * pressureCount) + s.press_abs) / (pressureCount++ + 1);
        break;
    }
    case 30:
        mavlink_attitude_t s;
        mavlink_msg_attitude_decode(m, &s);
        rollAvg = ((rollAvg * rollCount) + s.roll) / (rollCount++ + 1);
        pitchAvg = ((pitchAvg * pitchCount) + s.pitch) / (pitchCount++ + 1);
        yawAvg = ((yawAvg * yawCount) + s.yaw) / (yawCount++ + 1);
        rollRateAvg = ((rollRateAvg * rollRateCount) + s.rollspeed) / (rollRateCount++ + 1);
        pitchRateAvg = ((pitchRateAvg * pitchRateCount) + s.pitchspeed) / (pitchRateCount++ + 1);
        yawRateAvg = ((yawRateAvg * yawRateCount) + s.yawspeed) / (yawRateCount++ + 1);
        break;
    }

    /* Some fields not yet ready... */
    if (!(cassTemp0Count > 0 && cassTemp1Count > 0 && cassTemp1Count > 0 && cassRH0Count > 0 && cassRH1Count > 0 && cassRH2Count > 0 && altMmCount > 0 &&
          pressureCount > 0 && rollCount > 0 && pitchCount > 0 && yawCount > 0 && rollRateCount > 0 && pitchRateCount > 0 && yawRateCount > 0)) return;

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

    data.windBearingDegrees


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
    lastUpdate = currentTime;
}

#undef DEGREES
