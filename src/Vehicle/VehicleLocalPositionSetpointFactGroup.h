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

class VehicleLocalPositionSetpointFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleLocalPositionSetpointFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* x     READ x    CONSTANT)
    Q_PROPERTY(Fact* y     READ y    CONSTANT)
    Q_PROPERTY(Fact* z     READ z    CONSTANT)
    Q_PROPERTY(Fact* vx    READ vx   CONSTANT)
    Q_PROPERTY(Fact* vy    READ vy   CONSTANT)
    Q_PROPERTY(Fact* vz    READ vz   CONSTANT)

    // [m/s] Magnitude of the horizontal (XY) velocity calculated from vx and vy
    // Note: this field doesn't exist in the Local position message, but is calculated from vx and vy
    Q_PROPERTY(double vxy  READ vxy  CONSTANT)

    // Magnitude of the throttle command in XY plane, normalized (max thrust == 1.0)
    // Note: this field doesn't exist in the Local position message
    // Now I am hack-fully utilizing Acceleration-X field for sending this value,
    // since vehicle_thrust_setpoint uORB message isn't translated into MAVLink yet.
    Q_PROPERTY(double throttle  READ throttle  CONSTANT)

    Fact* x    () { return &_xFact; }
    Fact* y    () { return &_yFact; }
    Fact* z    () { return &_zFact; }
    Fact* vx   () { return &_vxFact; }
    Fact* vy   () { return &_vyFact; }
    Fact* vz   () { return &_vzFact; }

    double vxy  () { return _vxy; }
    double throttle  () { return _throttle; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _xFactName;
    static const char* _yFactName;
    static const char* _zFactName;
    static const char* _vxFactName;
    static const char* _vyFactName;
    static const char* _vzFactName;

private:
    Fact _xFact;
    Fact _yFact;
    Fact _zFact;
    Fact _vxFact;
    Fact _vyFact;
    Fact _vzFact;

    double _vxy;
    double _throttle;
};
