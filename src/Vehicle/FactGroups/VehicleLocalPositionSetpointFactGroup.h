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

    Fact* x    () { return &_xFact; }
    Fact* y    () { return &_yFact; }
    Fact* z    () { return &_zFact; }
    Fact* vx   () { return &_vxFact; }
    Fact* vy   () { return &_vyFact; }
    Fact* vz   () { return &_vzFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

private:
    const QString _xFactName =     QStringLiteral("x");
    const QString _yFactName =     QStringLiteral("y");
    const QString _zFactName =     QStringLiteral("z");
    const QString _vxFactName =    QStringLiteral("vx");
    const QString _vyFactName =    QStringLiteral("vy");
    const QString _vzFactName =    QStringLiteral("vz");

    Fact _xFact;
    Fact _yFact;
    Fact _zFact;
    Fact _vxFact;
    Fact _vyFact;
    Fact _vzFact;
};
