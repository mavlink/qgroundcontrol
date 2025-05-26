/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class VehicleFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *roll                   READ roll                   CONSTANT)
    Q_PROPERTY(Fact *pitch                  READ pitch                  CONSTANT)
    Q_PROPERTY(Fact *heading                READ heading                CONSTANT)
    Q_PROPERTY(Fact *rollRate               READ rollRate               CONSTANT)
    Q_PROPERTY(Fact *pitchRate              READ pitchRate              CONSTANT)
    Q_PROPERTY(Fact *yawRate                READ yawRate                CONSTANT)
    Q_PROPERTY(Fact *groundSpeed            READ groundSpeed            CONSTANT)
    Q_PROPERTY(Fact *airSpeed               READ airSpeed               CONSTANT)
    Q_PROPERTY(Fact *airSpeedSetpoint       READ airSpeedSetpoint       CONSTANT)
    Q_PROPERTY(Fact *climbRate              READ climbRate              CONSTANT)
    Q_PROPERTY(Fact *altitudeRelative       READ altitudeRelative       CONSTANT)
    Q_PROPERTY(Fact *altitudeAMSL           READ altitudeAMSL           CONSTANT)
    Q_PROPERTY(Fact *altitudeAboveTerr      READ altitudeAboveTerr      CONSTANT)
    Q_PROPERTY(Fact *altitudeTuning         READ altitudeTuning         CONSTANT)
    Q_PROPERTY(Fact *altitudeTuningSetpoint READ altitudeTuningSetpoint CONSTANT)
    Q_PROPERTY(Fact *xTrackError            READ xTrackError            CONSTANT)
    Q_PROPERTY(Fact *rangeFinderDist        READ rangeFinderDist        CONSTANT)
    Q_PROPERTY(Fact *flightDistance         READ flightDistance         CONSTANT)
    Q_PROPERTY(Fact *distanceToHome         READ distanceToHome         CONSTANT)
    Q_PROPERTY(Fact *timeToHome             READ timeToHome             CONSTANT)
    Q_PROPERTY(Fact *missionItemIndex       READ missionItemIndex       CONSTANT)
    Q_PROPERTY(Fact *headingToNextWP        READ headingToNextWP        CONSTANT)
    Q_PROPERTY(Fact *distanceToNextWP       READ distanceToNextWP       CONSTANT)
    Q_PROPERTY(Fact *headingToHome          READ headingToHome          CONSTANT)
    Q_PROPERTY(Fact *distanceToGCS          READ distanceToGCS          CONSTANT)
    Q_PROPERTY(Fact *hobbs                  READ hobbs                  CONSTANT)
    Q_PROPERTY(Fact *throttlePct            READ throttlePct            CONSTANT)
    Q_PROPERTY(Fact *imuTemp                READ imuTemp                CONSTANT)

public:
    explicit VehicleFactGroup(QObject *parent = nullptr);

    Fact *roll() { return &_rollFact; }
    Fact *pitch() { return &_pitchFact; }
    Fact *heading() { return &_headingFact; }
    Fact *rollRate() { return &_rollRateFact; }
    Fact *pitchRate() { return &_pitchRateFact; }
    Fact *yawRate() { return &_yawRateFact; }
    Fact *airSpeed() { return &_airSpeedFact; }
    Fact *airSpeedSetpoint() { return &_airSpeedSetpointFact; }
    Fact *groundSpeed() { return &_groundSpeedFact; }
    Fact *climbRate() { return &_climbRateFact; }
    Fact *altitudeRelative() { return &_altitudeRelativeFact; }
    Fact *altitudeAMSL() { return &_altitudeAMSLFact; }
    Fact *altitudeAboveTerr() { return &_altitudeAboveTerrFact; }
    Fact *altitudeTuning() { return &_altitudeTuningFact; }
    Fact *altitudeTuningSetpoint() { return &_altitudeTuningSetpointFact; }
    Fact *xTrackError() { return &_xTrackErrorFact; }
    Fact *rangeFinderDist() { return &_rangeFinderDistFact; }
    Fact *flightDistance() { return &_flightDistanceFact; }
    Fact *distanceToHome() { return &_distanceToHomeFact; }
    Fact *timeToHome() { return &_timeToHomeFact; }
    Fact *missionItemIndex() { return &_missionItemIndexFact; }
    Fact *headingToNextWP() { return &_headingToNextWPFact; }
    Fact *distanceToNextWP() { return &_distanceToNextWPFact; }
    Fact *headingToHome() { return &_headingToHomeFact; }
    Fact *distanceToGCS() { return &_distanceToGCSFact; }
    Fact *hobbs() { return &_hobbsFact; }
    Fact *throttlePct() { return &_throttlePctFact; }
    Fact *imuTemp() { return &_imuTempFact; }

    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

protected:
    void _handleAttitude(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleAttitudeQuaternion(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleAltitude(const mavlink_message_t &message);
    void _handleVfrHud(const mavlink_message_t &message);
    void _handleRawImuTemp(const mavlink_message_t &message);
    void _handleNavControllerOutput(const mavlink_message_t &message);
#ifndef QGC_NO_ARDUPILOT_DIALECT
    void _handleRangefinder(const mavlink_message_t &message);
#endif

    Fact _rollFact = Fact(0, QStringLiteral("roll"), FactMetaData::valueTypeDouble);
    Fact _pitchFact = Fact(0, QStringLiteral("pitch"), FactMetaData::valueTypeDouble);
    Fact _headingFact = Fact(0, QStringLiteral("heading"), FactMetaData::valueTypeDouble);
    Fact _rollRateFact = Fact(0, QStringLiteral("rollRate"), FactMetaData::valueTypeDouble);
    Fact _pitchRateFact = Fact(0, QStringLiteral("pitchRate"), FactMetaData::valueTypeDouble);
    Fact _yawRateFact = Fact(0, QStringLiteral("yawRate"), FactMetaData::valueTypeDouble);
    Fact _groundSpeedFact = Fact(0, QStringLiteral("groundSpeed"), FactMetaData::valueTypeDouble);
    Fact _airSpeedFact = Fact(0, QStringLiteral("airSpeed"), FactMetaData::valueTypeDouble);
    Fact _airSpeedSetpointFact = Fact(0, QStringLiteral("airSpeedSetpoint"), FactMetaData::valueTypeDouble);
    Fact _climbRateFact = Fact(0, QStringLiteral("climbRate"), FactMetaData::valueTypeDouble);
    Fact _altitudeRelativeFact = Fact(0, QStringLiteral("altitudeRelative"), FactMetaData::valueTypeDouble);
    Fact _altitudeAMSLFact = Fact(0, QStringLiteral("altitudeAMSL"), FactMetaData::valueTypeDouble);
    Fact _altitudeAboveTerrFact = Fact(0, QStringLiteral("altitudeAboveTerr"), FactMetaData::valueTypeDouble);
    Fact _altitudeTuningFact = Fact(0, QStringLiteral("altitudeTuning"), FactMetaData::valueTypeDouble);
    Fact _altitudeTuningSetpointFact = Fact(0, QStringLiteral("altitudeTuningSetpoint"), FactMetaData::valueTypeDouble);
    Fact _xTrackErrorFact = Fact(0, QStringLiteral("xTrackError"), FactMetaData::valueTypeDouble);
    Fact _rangeFinderDistFact = Fact(0, QStringLiteral("rangeFinderDist"), FactMetaData::valueTypeFloat);
    Fact _flightDistanceFact = Fact(0, QStringLiteral("flightDistance"), FactMetaData::valueTypeDouble);
    Fact _flightTimeFact = Fact(0, QStringLiteral("flightTime"), FactMetaData::valueTypeElapsedTimeInSeconds);
    Fact _distanceToHomeFact = Fact(0, QStringLiteral("distanceToHome"), FactMetaData::valueTypeDouble);
    Fact _timeToHomeFact = Fact(0, QStringLiteral("timeToHome"), FactMetaData::valueTypeDouble);
    Fact _missionItemIndexFact = Fact(0, QStringLiteral("missionItemIndex"), FactMetaData::valueTypeUint16);
    Fact _headingToNextWPFact = Fact(0, QStringLiteral("headingToNextWP"), FactMetaData::valueTypeDouble);
    Fact _distanceToNextWPFact = Fact(0, QStringLiteral("distanceToNextWP"), FactMetaData::valueTypeDouble);
    Fact _headingToHomeFact = Fact(0, QStringLiteral("headingToHome"), FactMetaData::valueTypeDouble);
    Fact _distanceToGCSFact = Fact(0, QStringLiteral("distanceToGCS"), FactMetaData::valueTypeDouble);
    Fact _hobbsFact = Fact(0, QStringLiteral("hobbs"), FactMetaData::valueTypeString);
    Fact _throttlePctFact = Fact(0, QStringLiteral("throttlePct"), FactMetaData::valueTypeUint16);
    Fact _imuTempFact = Fact(0, QStringLiteral("imuTemp"), FactMetaData::valueTypeInt16);

    float _altitudeTuningOffset = qQNaN();

protected:
    bool _altitudeMessageAvailable = false;

private:
    void _handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians);

    bool _receivingAttitudeQuaternion = false;
};
