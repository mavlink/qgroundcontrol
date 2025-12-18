/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class VehicleRPMSensorFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *rpm1 READ rpm1 CONSTANT)
    Q_PROPERTY(Fact *rpm2 READ rpm2 CONSTANT)

public:
    explicit VehicleRPMSensorFactGroup(QObject *parent = nullptr);

    Fact *rpm1() { return &_rpm1SensorFact; }
    Fact *rpm2() { return &_rpm2SensorFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleRPMSensor(const mavlink_message_t &message);

    Fact _rpm1SensorFact = Fact(0, QStringLiteral("rpm1"), FactMetaData::valueTypeDouble);
    Fact _rpm2SensorFact = Fact(0, QStringLiteral("rpm2"), FactMetaData::valueTypeDouble);
};
