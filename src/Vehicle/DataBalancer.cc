#pragma once

#include "DataBalancer.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "VehicleTemperatureFactGroup.h"
#include "MetDataLogManager.h"
#include <chrono>

#define _CRT_SECURE_NO_WARNINGS
#define DEGREES(radians) (radians * (180.0 / M_PI))
#define RADIANS(degrees) (degrees * (M_PI / 180))
#define EPSILON 1e-8

void DataBalancer::calcWindProps(IMetData* d){
    float croll = cos(d->rollRadians);
    float sroll = sin(d->rollRadians);
    float cpitch = cos(d->pitchRadians);
    float spitch = sin(d->pitchRadians);
    float cyaw = cos(d->yawRadians);
    float syaw = sin(d->yawRadians);

    d->windBearingDegrees = fmodf(DEGREES(atan2(-croll * spitch * syaw + sroll * cyaw, -sroll * syaw - croll * spitch * cyaw)) + 360.f, 360.f);
    d->windSpeedMetersPerSecond = fmaxf(0.f, 39.4f * sqrt(tan(acos(croll * cpitch))) - 5.71f);
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
            cassTemp0A = ((cassTemp0A * cassTemp0C) + s.values[0]) / (int64_t)(cassTemp0C + 1);
            cassTemp1A = ((cassTemp1A * cassTemp1C) + s.values[1]) / (int64_t)(cassTemp1C + 1);
            cassTemp2A = ((cassTemp2A * cassTemp2C) + s.values[2]) / (int64_t)(cassTemp2C + 1);
            cassTemp0C++;
            cassTemp1C++;
            cassTemp2C++;
            break;
        }
        case 1:{ /* iMet RH */            
            cassRH0A = ((cassRH0A * cassRH0C) + s.values[0]) / (int64_t)(cassRH0C + 1);
            cassRH1A = ((cassRH1A * cassRH1C) + s.values[1]) / (int64_t)(cassRH1C + 1);
            cassRH2A = ((cassRH2A * cassRH2C) + s.values[2]) / (int64_t)(cassRH2C + 1);
            cassRH0C++;
            cassRH1C++;
            cassRH2C++;
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
        xVelocityA = ((xVelocityA * xVelocityC) + s.vx) / (int64_t)(xVelocityC + 1);
        yVelocityA = ((yVelocityA * yVelocityC) + s.vy) / (int64_t)(yVelocityC + 1);
        zVelocityA = ((zVelocityA * zVelocityC) + s.vz) / (int64_t)(zVelocityC + 1);
        xVelocityC++;
        yVelocityC++;
        zVelocityC++;
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:{
        mavlink_global_position_int_t s;
        mavlink_msg_global_position_int_decode(m, &s);
        altMmA =     ((int64_t)(altMmA     * altMmC)     + s.alt) / (int64_t)(altMmC + 1);
        latitudeDegreesE7A =  ((int64_t)(latitudeDegreesE7A  * latitudeDegreesE7C)  + s.lat) / (int64_t)(latitudeDegreesE7C + 1);
        headingGlobalCentidegreesA = ((uint32_t)(headingGlobalCentidegreesA * headingGlobalCentidegreesC) + s.hdg) / (uint32_t)(headingGlobalCentidegreesC + 1);

        //handle averaging near 180th meridian
        int32_t sLonNormalized = s.lon;
        if (longitudeDegreesE7C > 0 && (longitudeDegreesE7A - s.lon) > 180e7) {
            sLonNormalized += 360e7;
        } else if (longitudeDegreesE7C > 0 && (longitudeDegreesE7A - s.lon) < -180e7) {
            sLonNormalized -= 360e7;
        }

        longitudeDegreesE7A = ((int64_t)(longitudeDegreesE7A * longitudeDegreesE7C) + sLonNormalized) / (int64_t)(longitudeDegreesE7C + 1);
        altMmC++;
        latitudeDegreesE7C++;
        longitudeDegreesE7C++;
        headingGlobalCentidegreesC++;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE2:{
        mavlink_scaled_pressure2_t s;
        mavlink_msg_scaled_pressure2_decode(m, &s);
        absolutePressureMillibarsA = ((absolutePressureMillibarsA * absolutePressureMillibarsC) + s.press_abs) / (absolutePressureMillibarsC + 1);
        absolutePressureMillibarsC++;
        break;
    }
    case MAVLINK_MSG_ID_ATTITUDE:{
        mavlink_attitude_t s;
        mavlink_msg_attitude_decode(m, &s);
        rollRadiansA =      ((rollRadiansA      * rollRadiansC)      + s.roll)       / (int64_t)(rollRadiansC + 1);
        pitchRadiansA =     ((pitchRadiansA     * pitchRadiansC)     + s.pitch)      / (int64_t)(pitchRadiansC + 1);
        yawRadiansA =       ((yawRadiansA       * yawRadiansC)       + s.yaw)        / (int64_t)(yawRadiansC + 1);
        rollRateRadiansPerSecondA =  ((rollRateRadiansPerSecondA  * rollRateRadiansPerSecondC)  + s.rollspeed)  / (int64_t)(rollRateRadiansPerSecondC + 1);
        pitchRateRadiansPerSecondA = ((pitchRateRadiansPerSecondA * pitchRateRadiansPerSecondC) + s.pitchspeed) / (int64_t)(pitchRateRadiansPerSecondC + 1);
        yawRateRadiansPerSecondA =   ((yawRateRadiansPerSecondA   * yawRateRadiansPerSecondC)   + s.yawspeed)   / (int64_t)(yawRateRadiansPerSecondC + 1);
        rollRadiansC++;
        pitchRadiansC++;
        yawRadiansC++;
        rollRateRadiansPerSecondC++;
        pitchRateRadiansPerSecondC++;
        yawRateRadiansPerSecondC++;
        break;
    }
    case MAVLINK_MSG_ID_HEARTBEAT:{
        mavlink_heartbeat_t s;
        mavlink_msg_heartbeat_decode(m, &s);
        data.heartBeatCustomMode = s.custom_mode;
    }
    }

    /* Some fields not yet ready... */
    if (!(cassTemp0C > 0 && cassTemp1C > 0 && cassTemp2C > 0 && cassRH0C > 0 && cassRH1C > 0 && cassRH2C > 0 && altMmC > 0 &&
          absolutePressureMillibarsC > 0 && rollRadiansC > 0 && pitchRadiansC > 0 && yawRadiansC > 0 && rollRateRadiansPerSecondC > 0 && pitchRateRadiansPerSecondC > 0 && yawRateRadiansPerSecondC > 0 &&
          latitudeDegreesE7C > 0 && longitudeDegreesE7C > 0 && zVelocityC > 0 && xVelocityC > 0 && yVelocityC > 0 && headingGlobalCentidegreesC > 0)
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
    data.altitudeMillimetersMSL = altMmA;
    data.altitudeMetersMSL = (double)altMmA / 1000;
    data.absolutePressureMillibars = absolutePressureMillibarsA;
    data.temperature0Kelvin = cassTemp0A;
    data.temperature1Kelvin = cassTemp1A;
    data.temperature2Kelvin = cassTemp2A;
    data.temperatureCelsius = ((cassTemp0A + cassTemp1A + cassTemp2A) / 3) - 273.15;
    data.relativeHumidity0 = cassRH0A;
    data.relativeHumidity1 = cassRH1A;
    data.relativeHumidity2 = cassRH2A;
    data.relativeHumidity = (cassRH0A + cassRH1A + cassRH2A) / 3;
    data.rollRadians = rollRadiansA;
    data.pitchRadians = pitchRadiansA;
    data.yawRadians = yawRadiansA;
    data.headingGlobalCentidegrees = headingGlobalCentidegreesA;
    data.rollRateRadiansPerSecond = rollRateRadiansPerSecondA;
    data.pitchRateRadiansPerSecond = pitchRateRadiansPerSecondA;
    data.yawRateRadiansPerSecond = yawRateRadiansPerSecondA;
    data.rollDegrees = DEGREES(data.rollRadians);
    data.pitchDegrees = DEGREES(data.pitchRadians);
    data.yawDegrees = DEGREES(data.yawRadians);
    data.rollRateDegreesPerSecond = DEGREES(data.rollRateRadiansPerSecond);
    data.pitchRateDegreesPerSecond = DEGREES(data.pitchRateRadiansPerSecond);
    data.yawRateDegreesPerSecond = DEGREES(data.yawRateRadiansPerSecond);
    calcWindProps(&data);
    data.latitudeDegreesE7 = latitudeDegreesE7A;
    data.longitudeDegreesE7 = longitudeDegreesE7A;
    data.latitudeDegrees = (double)latitudeDegreesE7A / 1e7;
    data.longitudeDegrees = (double)longitudeDegreesE7A / 1e7;
    data.zVelocityMetersPerSecondInverted = zVelocityA;
    data.xVelocityMetersPerSecond = xVelocityA;
    data.yVelocityMetersPerSecond = yVelocityA;
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
        qgcApp()->toolbox()->metDataLogManager()->setAscentNumber(data.ascents);
    }

    if (!data.ascending && data.lastState){
        resetALM();
    }

    data.lastState = data.ascending;

    ascents->setRawValue(data.ascents);
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
    cassTemp0C = 0;
    cassTemp1C = 0;
    cassTemp2C = 0;
    cassRH0C = 0;
    cassRH1C = 0;
    cassRH2C = 0;
    altMmC = 0;
    absolutePressureMillibarsC = 0;
    rollC = 0;
    pitchRadiansC = 0;
    yawRadiansC = 0;
    headingGlobalCentidegreesC = 0;
    rollRateRadiansPerSecondC = 0;
    pitchRateRadiansPerSecondC = 0;
    yawRateRadiansPerSecondC = 0;
    latitudeDegreesE7C = 0;
    longitudeDegreesE7C = 0;
    zVelocityC = 0;
    xVelocityC = 0;
    yVelocityC = 0;
    lastUpdate = currentTime;
}

int DataBalancer::updateALM(){
    int errorCode = DATA_NOT_INITIALIZED;
    if (!dataInit) return errorCode;
    errorCode = NOT_ASCENDING;
    if (!data.ascending) return errorCode;

    if (!ALMInit) {
        ALMInit = 1;
        // lowBinAlt is rounded down to nearest binsize from altitudeMetersMSL
        lowBinAlt = (float)floor(data.altitudeMetersMSL / binSize) * binSize;
    }

    aslA = ((aslA * aslC) + data.altitudeMetersMSL) / (aslC + 1);
    timeA = ((timeA * timeC) + data.timeUnixSeconds) / (timeC + 1);
    pressureA = ((pressureA * pressureC) + data.absolutePressureMillibars) / (pressureC + 1);
    airTempA = ((airTempA * airTempC) + data.temperatureCelsius) / (airTempC + 1);
    relHumA = ((relHumA * relHumC) + data.relativeHumidity) / (relHumC + 1);
    windSpeedA = ((windSpeedA * windSpeedC) + data.windSpeedMetersPerSecond) / (windSpeedC + 1);
    windDirectionA = ((windDirectionA * windDirectionC) + data.windBearingDegrees) / (windDirectionC + 1);
    latitudeA = ((latitudeA * latitudeC) + data.latitudeDegrees) / (latitudeC + 1);
    longitudeA = ((longitudeA * longitudeC) + data.longitudeDegrees) / (longitudeC + 1);
    rollA = ((rollA * rollC) + data.rollDegrees) / (rollC + 1);
    rollRateA = ((rollRateA * rollRateC) + data.rollRateDegreesPerSecond) / (rollRateC + 1);
    pitchA = ((pitchA * pitchC) + data.pitchDegrees) / (pitchC + 1);
    pitchRateA = ((pitchRateA * pitchRateC) + data.pitchRateDegreesPerSecond) / (pitchRateC + 1);
    yawA = ((yawA * yawC) + data.yawDegrees) / (yawC + 1);
    yawRateA = ((yawRateA * yawRateC) + data.yawRateDegreesPerSecond) / (yawRateC + 1);
    ascentRateA = ((ascentRateA * ascentRateC) + data.zVelocityMetersPerSecond) / (ascentRateC + 1);
    speedOverGroundA = ((speedOverGroundA * speedOverGroundC) + data.groundSpeedMetersPerSecond) / (speedOverGroundC + 1);

    aslC++;
    timeC++;
    pressureC++;
    airTempC++;
    relHumC++;
    windSpeedC++;
    windDirectionC++;
    latitudeC++;
    longitudeC++;
    rollC++;
    rollRateC++;
    pitchC++;
    pitchRateC++;
    yawC++;
    yawRateC++;
    ascentRateC++;
    speedOverGroundC++;

    errorCode = ALTITUDE_CHANGE_TOO_SMALL;
    if (abs(data.altitudeMetersMSL - lowBinAlt) < binSize) return errorCode;

    alm.asl = (float)ceil(aslA / binSize) * binSize - binSize / 2;
    alm.time = timeA;
    alm.pressure = pressureA;
    alm.airTemp = airTempA;
    alm.relHum = relHumA;
    alm.windSpeed = windSpeedA;
    alm.windDirection = windDirectionA;
    alm.latitude = latitudeA;
    alm.longitude = longitudeA;
    alm.roll = rollA;
    alm.rollRate = rollRateA;
    alm.pitch = pitchA;
    alm.pitchRate = pitchRateA;
    alm.yaw = yawA;
    alm.yawRate = yawRateA;
    alm.ascentRate = ascentRateA;
    alm.speedOverGround = speedOverGroundA;

    aslC = 0;
    timeC = 0;
    pressureC = 0;
    airTempC = 0;
    relHumC = 0;
    windSpeedC = 0;
    windDirectionC = 0;
    latitudeC = 0;
    longitudeC = 0;
    rollC = 0;
    rollRateC = 0;
    pitchC = 0;
    pitchRateC = 0;
    yawC = 0;
    yawRateC = 0;
    ascentRateC = 0;
    speedOverGroundC = 0;
    lowBinAlt = (float)floor(data.altitudeMetersMSL / binSize) * binSize;

    /* Success */
    errorCode = SUCCESS;
    return errorCode;
}

void DataBalancer::onALMUpdate(Fact* asl, Fact* time, Fact* pressure, Fact* airTemp, Fact* relHum, Fact* windSpeed, Fact* windDirection, Fact* latitude, Fact* longitude,
                               Fact* roll, Fact* rollRate, Fact* pitch, Fact* pitchRate, Fact* yaw, Fact* yawRate, Fact* ascentRate, Fact* speedOverGround,
                               Fact* update){
    asl->setRawValue(alm.asl);
    time->setRawValue(alm.time);
    pressure->setRawValue(alm.pressure);
    airTemp->setRawValue(alm.airTemp);
    relHum->setRawValue(alm.relHum);
    windSpeed->setRawValue(alm.windSpeed);
    windDirection->setRawValue(alm.windDirection);
    latitude->setRawValue(alm.latitude);
    longitude->setRawValue(alm.longitude);
    roll->setRawValue(alm.roll);
    rollRate->setRawValue(alm.rollRate);
    pitch->setRawValue(alm.pitch);
    pitchRate->setRawValue(alm.pitchRate);
    yaw->setRawValue(alm.yaw);
    yawRate->setRawValue(alm.yawRate);
    ascentRate->setRawValue(alm.ascentRate);
    speedOverGround->setRawValue(alm.speedOverGround);

    update->setRawValue(1);
}

void DataBalancer::resetALM(){
    ALMInit = 0;
    aslC = 0;
    timeC = 0;
    pressureC = 0;
    airTempC = 0;
    relHumC = 0;
    windSpeedC = 0;
    windDirectionC = 0;
    latitudeC = 0;
    longitudeC = 0;
    rollC = 0;
    rollRateC = 0;
    pitchC = 0;
    pitchRateC = 0;
    yawC = 0;
    yawRateC = 0;
    ascentRateC = 0;
    speedOverGroundC = 0;
}

#undef DEGREES
#undef RADIANS
#undef EPSILON
