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
#include "QGCMAVLink.h"

class VehicleSetpointFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleSetpointFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* roll       READ roll       CONSTANT)
    Q_PROPERTY(Fact* pitch      READ pitch      CONSTANT)
    Q_PROPERTY(Fact* yaw        READ yaw        CONSTANT)
    Q_PROPERTY(Fact* rollRate   READ rollRate   CONSTANT)
    Q_PROPERTY(Fact* pitchRate  READ pitchRate  CONSTANT)
    Q_PROPERTY(Fact* yawRate    READ yawRate    CONSTANT)

    Fact* roll      () { return &_rollFact; }
    Fact* pitch     () { return &_pitchFact; }
    Fact* yaw       () { return &_yawFact; }
    Fact* rollRate  () { return &_rollRateFact; }
    Fact* pitchRate () { return &_pitchRateFact; }
    Fact* yawRate   () { return &_yawRateFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

private:
    const QString _rollFactName =       QStringLiteral("roll");
    const QString _pitchFactName =      QStringLiteral("pitch");
    const QString _yawFactName =        QStringLiteral("yaw");
    const QString _rollRateFactName =   QStringLiteral("rollRate");
    const QString _pitchRateFactName =  QStringLiteral("pitchRate");
    const QString _yawRateFactName =    QStringLiteral("yawRate");

    Fact _rollFact;
    Fact _pitchFact;
    Fact _yawFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
};
