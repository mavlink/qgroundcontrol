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
#include "MAVLinkLib.h"

class VehicleFactGroup : public FactGroup
{
    Q_OBJECT

    Q_PROPERTY(Fact* roll                   READ roll                   CONSTANT)
    Q_PROPERTY(Fact* pitch                  READ pitch                  CONSTANT)
    Q_PROPERTY(Fact* heading                READ heading                CONSTANT)
    Q_PROPERTY(Fact* rollRate               READ rollRate               CONSTANT)
    Q_PROPERTY(Fact* pitchRate              READ pitchRate              CONSTANT)
    Q_PROPERTY(Fact* yawRate                READ yawRate                CONSTANT)
    Q_PROPERTY(Fact* groundSpeed            READ groundSpeed            CONSTANT)
    Q_PROPERTY(Fact* airSpeed               READ airSpeed               CONSTANT)
    Q_PROPERTY(Fact* airSpeedSetpoint       READ airSpeedSetpoint       CONSTANT)
    Q_PROPERTY(Fact* climbRate              READ climbRate              CONSTANT)
    Q_PROPERTY(Fact* altitudeRelative       READ altitudeRelative       CONSTANT)
    Q_PROPERTY(Fact* altitudeAMSL           READ altitudeAMSL           CONSTANT)
    Q_PROPERTY(Fact* altitudeAboveTerr      READ altitudeAboveTerr      CONSTANT)
    Q_PROPERTY(Fact* altitudeTuning         READ altitudeTuning         CONSTANT)
    Q_PROPERTY(Fact* altitudeTuningSetpoint READ altitudeTuningSetpoint CONSTANT)
    Q_PROPERTY(Fact* xTrackError            READ xTrackError            CONSTANT)
    Q_PROPERTY(Fact* rangeFinderDist        READ rangeFinderDist        CONSTANT)
    Q_PROPERTY(Fact* flightDistance         READ flightDistance         CONSTANT)
    Q_PROPERTY(Fact* distanceToHome         READ distanceToHome         CONSTANT)
    Q_PROPERTY(Fact* timeToHome             READ timeToHome             CONSTANT)
    Q_PROPERTY(Fact* missionItemIndex       READ missionItemIndex       CONSTANT)
    Q_PROPERTY(Fact* headingToNextWP        READ headingToNextWP        CONSTANT)
    Q_PROPERTY(Fact* distanceToNextWP       READ distanceToNextWP       CONSTANT)
    Q_PROPERTY(Fact* headingToHome          READ headingToHome          CONSTANT)
    Q_PROPERTY(Fact* distanceToGCS          READ distanceToGCS          CONSTANT)
    Q_PROPERTY(Fact* hobbs                  READ hobbs                  CONSTANT)
    Q_PROPERTY(Fact* throttlePct            READ throttlePct            CONSTANT)
    Q_PROPERTY(Fact* imuTemp                READ imuTemp                CONSTANT)

public:
    VehicleFactGroup(QObject* parent = nullptr);

    Fact* roll                      () { return &_rollFact; }
    Fact* pitch                     () { return &_pitchFact; }
    Fact* heading                   () { return &_headingFact; }
    Fact* rollRate                  () { return &_rollRateFact; }
    Fact* pitchRate                 () { return &_pitchRateFact; }
    Fact* yawRate                   () { return &_yawRateFact; }
    Fact* airSpeed                  () { return &_airSpeedFact; }
    Fact* airSpeedSetpoint          () { return &_airSpeedSetpointFact; }
    Fact* groundSpeed               () { return &_groundSpeedFact; }
    Fact* climbRate                 () { return &_climbRateFact; }
    Fact* altitudeRelative          () { return &_altitudeRelativeFact; }
    Fact* altitudeAMSL              () { return &_altitudeAMSLFact; }
    Fact* altitudeAboveTerr         () { return &_altitudeAboveTerrFact; }
    Fact* altitudeTuning            () { return &_altitudeTuningFact; }
    Fact* altitudeTuningSetpoint    () { return &_altitudeTuningSetpointFact; }
    Fact* xTrackError               () { return &_xTrackErrorFact; }
    Fact* rangeFinderDist           () { return &_rangeFinderDistFact; }
    Fact* flightDistance            () { return &_flightDistanceFact; }
    Fact* distanceToHome            () { return &_distanceToHomeFact; }
    Fact* timeToHome                () { return &_timeToHomeFact; }
    Fact* missionItemIndex          () { return &_missionItemIndexFact; }
    Fact* headingToNextWP           () { return &_headingToNextWPFact; }
    Fact* distanceToNextWP          () { return &_distanceToNextWPFact; }
    Fact* headingToHome             () { return &_headingToHomeFact; }
    Fact* distanceToGCS             () { return &_distanceToGCSFact; }
    Fact* hobbs                     () { return &_hobbsFact; }
    Fact* throttlePct               () { return &_throttlePctFact; }
    Fact* imuTemp                   () { return &_imuTempFact; }

    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

protected:
    void _handleAttitude                (Vehicle* vehicle, const mavlink_message_t &message);
    void _handleAttitudeQuaternion      (Vehicle* vehicle, const mavlink_message_t &message);
    void _handleAltitude                (const mavlink_message_t &message);
    void _handleVfrHud                  (const mavlink_message_t &message);
    void _handleRawImuTemp              (const mavlink_message_t &message);
    void _handleNavControllerOutput     (const mavlink_message_t &message);
#ifndef QGC_NO_ARDUPILOT_DIALECT
    void _handleRangefinder             (const mavlink_message_t &message);
#endif

    const QString _rollFactName =                   QStringLiteral("roll");
    const QString _pitchFactName =                  QStringLiteral("pitch");
    const QString _headingFactName =                QStringLiteral("heading");
    const QString _rollRateFactName =               QStringLiteral("rollRate");
    const QString _pitchRateFactName =              QStringLiteral("pitchRate");
    const QString _yawRateFactName =                QStringLiteral("yawRate");
    const QString _airSpeedFactName =               QStringLiteral("airSpeed");
    const QString _airSpeedSetpointFactName =       QStringLiteral("airSpeedSetpoint");
    const QString _xTrackErrorFactName =            QStringLiteral("xTrackError");
    const QString _rangeFinderDistFactName =        QStringLiteral("rangeFinderDist");
    const QString _groundSpeedFactName =            QStringLiteral("groundSpeed");
    const QString _climbRateFactName =              QStringLiteral("climbRate");
    const QString _altitudeRelativeFactName =       QStringLiteral("altitudeRelative");
    const QString _altitudeAMSLFactName =           QStringLiteral("altitudeAMSL");
    const QString _altitudeAboveTerrFactName =      QStringLiteral("altitudeAboveTerr");
    const QString _altitudeTuningFactName =         QStringLiteral("altitudeTuning");
    const QString _altitudeTuningSetpointFactName = QStringLiteral("altitudeTuningSetpoint");
    const QString _flightDistanceFactName =         QStringLiteral("flightDistance");
    const QString _flightTimeFactName =             QStringLiteral("flightTime");
    const QString _distanceToHomeFactName =         QStringLiteral("distanceToHome");
    const QString _timeToHomeFactName =             QStringLiteral("timeToHome");
    const QString _missionItemIndexFactName =       QStringLiteral("missionItemIndex");
    const QString _headingToNextWPFactName =        QStringLiteral("headingToNextWP");
    const QString _distanceToNextWPFactName =       QStringLiteral("distanceToNextWP");
    const QString _headingToHomeFactName =          QStringLiteral("headingToHome");
    const QString _distanceToGCSFactName =          QStringLiteral("distanceToGCS");
    const QString _hobbsFactName =                  QStringLiteral("hobbs");
    const QString _throttlePctFactName =            QStringLiteral("throttlePct");
    const QString _imuTempFactName =                QStringLiteral("imuTemp");

    Fact _rollFact;
    Fact _pitchFact;
    Fact _headingFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
    Fact _groundSpeedFact;
    Fact _airSpeedFact;
    Fact _airSpeedSetpointFact;
    Fact _climbRateFact;
    Fact _altitudeRelativeFact;
    Fact _altitudeAMSLFact;
    Fact _altitudeAboveTerrFact;
    Fact _altitudeTuningFact;
    Fact _altitudeTuningSetpointFact;
    Fact _xTrackErrorFact;
    Fact _rangeFinderDistFact;
    Fact _flightDistanceFact;
    Fact _flightTimeFact;
    Fact _distanceToHomeFact;
    Fact _timeToHomeFact;
    Fact _missionItemIndexFact;
    Fact _headingToNextWPFact;
    Fact _distanceToNextWPFact;
    Fact _headingToHomeFact;
    Fact _distanceToGCSFact;
    Fact _hobbsFact;
    Fact _throttlePctFact;
    Fact _imuTempFact;

    float _altitudeTuningOffset = qQNaN(); // altitude offset, so the plotted value is around 0

protected:
    bool _altitudeMessageAvailable = false;

private:
    void _handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians);

    bool _receivingAttitudeQuaternion = false;
    static constexpr int _vehicleUIUpdateRateMSecs = 100;
};
