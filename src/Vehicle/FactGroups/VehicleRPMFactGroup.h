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

class VehicleRPMFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *rpm1 READ rpm1 CONSTANT)
    Q_PROPERTY(Fact *rpm2 READ rpm2 CONSTANT)
    Q_PROPERTY(Fact *rpm3 READ rpm3 CONSTANT)
    Q_PROPERTY(Fact *rpm4 READ rpm4 CONSTANT)
    Q_PROPERTY(Fact *rpmSensor1 READ rpmSensor1 CONSTANT)
    Q_PROPERTY(Fact *rpmSensor2 READ rpmSensor2 CONSTANT)

public:
    explicit VehicleRPMFactGroup(QObject *parent = nullptr);

    Fact *rpm1() { return &_rpm1Fact; }
    Fact *rpm2() { return &_rpm2Fact; }
    Fact *rpm3() { return &_rpm3Fact; }
    Fact *rpm4() { return &_rpm4Fact; }
    Fact *rpmSensor1() { return &_rpmSensor1Fact; }
    Fact *rpmSensor2() { return &_rpmSensor2Fact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleRawRPM(const mavlink_message_t &message);
    void _handleRPMSensor(const mavlink_message_t &message);

    Fact _rpm1Fact = Fact(0, QStringLiteral("rpm1"), FactMetaData::valueTypeDouble);
    Fact _rpm2Fact = Fact(0, QStringLiteral("rpm2"), FactMetaData::valueTypeDouble);
    Fact _rpm3Fact = Fact(0, QStringLiteral("rpm3"), FactMetaData::valueTypeDouble);
    Fact _rpm4Fact = Fact(0, QStringLiteral("rpm4"), FactMetaData::valueTypeDouble);
    Fact _rpmSensor1Fact = Fact(0, QStringLiteral("rpmSensor1"), FactMetaData::valueTypeDouble);
    Fact _rpmSensor2Fact = Fact(0, QStringLiteral("rpmSensor2"), FactMetaData::valueTypeDouble);
};
