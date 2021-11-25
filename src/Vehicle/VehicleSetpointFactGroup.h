/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    static constexpr const char* _rollFactName        = "roll";
    static constexpr const char* _pitchFactName       = "pitch";
    static constexpr const char* _yawFactName         = "yaw";
    static constexpr const char* _rollRateFactName    = "rollRate";
    static constexpr const char* _pitchRateFactName   = "pitchRate";
    static constexpr const char* _yawRateFactName     = "yawRate";

private:
    Fact _rollFact;
    Fact _pitchFact;
    Fact _yawFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
};
