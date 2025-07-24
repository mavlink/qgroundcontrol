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

class VehicleSetpointFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *roll       READ roll       CONSTANT)
    Q_PROPERTY(Fact *pitch      READ pitch      CONSTANT)
    Q_PROPERTY(Fact *yaw        READ yaw        CONSTANT)
    Q_PROPERTY(Fact *rollRate   READ rollRate   CONSTANT)
    Q_PROPERTY(Fact *pitchRate  READ pitchRate  CONSTANT)
    Q_PROPERTY(Fact *yawRate    READ yawRate    CONSTANT)

public:
    explicit VehicleSetpointFactGroup(QObject *parent = nullptr);

    Fact *roll() { return &_rollFact; }
    Fact *pitch() { return &_pitchFact; }
    Fact *yaw() { return &_yawFact; }
    Fact *rollRate() { return &_rollRateFact; }
    Fact *pitchRate() { return &_pitchRateFact; }
    Fact *yawRate() { return &_yawRateFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _rollFact = Fact(0, QStringLiteral("roll"), FactMetaData::valueTypeDouble);
    Fact _pitchFact = Fact(0, QStringLiteral("pitch"), FactMetaData::valueTypeDouble);
    Fact _yawFact = Fact(0, QStringLiteral("yaw"), FactMetaData::valueTypeDouble);
    Fact _rollRateFact = Fact(0, QStringLiteral("rollRate"), FactMetaData::valueTypeDouble);
    Fact _pitchRateFact = Fact(0, QStringLiteral("pitchRate"), FactMetaData::valueTypeDouble);
    Fact _yawRateFact = Fact(0, QStringLiteral("yawRate"), FactMetaData::valueTypeDouble);
};
