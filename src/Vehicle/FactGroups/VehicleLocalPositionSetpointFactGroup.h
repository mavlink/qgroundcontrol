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

class VehicleLocalPositionSetpointFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *x  READ x  CONSTANT)
    Q_PROPERTY(Fact *y  READ y  CONSTANT)
    Q_PROPERTY(Fact *z  READ z  CONSTANT)
    Q_PROPERTY(Fact *vx READ vx CONSTANT)
    Q_PROPERTY(Fact *vy READ vy CONSTANT)
    Q_PROPERTY(Fact *vz READ vz CONSTANT)

public:
    explicit VehicleLocalPositionSetpointFactGroup(QObject *parent = nullptr);

    Fact *x() { return &_xFact; }
    Fact *y() { return &_yFact; }
    Fact *z() { return &_zFact; }
    Fact *vx() { return &_vxFact; }
    Fact *vy() { return &_vyFact; }
    Fact *vz() { return &_vzFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _xFact = Fact(0, QStringLiteral("x"), FactMetaData::valueTypeDouble);
    Fact _yFact = Fact(0, QStringLiteral("y"), FactMetaData::valueTypeDouble);
    Fact _zFact = Fact(0, QStringLiteral("z"), FactMetaData::valueTypeDouble);
    Fact _vxFact = Fact(0, QStringLiteral("vx"), FactMetaData::valueTypeDouble);
    Fact _vyFact = Fact(0, QStringLiteral("vy"), FactMetaData::valueTypeDouble);
    Fact _vzFact = Fact(0, QStringLiteral("vz"), FactMetaData::valueTypeDouble);
};
