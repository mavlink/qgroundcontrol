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

class VehicleTemperatureFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *temperature1 READ temperature1 CONSTANT)
    Q_PROPERTY(Fact *temperature2 READ temperature2 CONSTANT)
    Q_PROPERTY(Fact *temperature3 READ temperature3 CONSTANT)

public:
    explicit VehicleTemperatureFactGroup(QObject *parent = nullptr);

    Fact *temperature1() { return &_temperature1Fact; }
    Fact *temperature2() { return &_temperature2Fact; }
    Fact *temperature3() { return &_temperature3Fact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleScaledPressure(const mavlink_message_t &message);
    void _handleScaledPressure2(const mavlink_message_t &message);
    void _handleScaledPressure3(const mavlink_message_t &message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);

    Fact _temperature1Fact = Fact(0, QStringLiteral("temperature1"), FactMetaData::valueTypeDouble);
    Fact _temperature2Fact = Fact(0, QStringLiteral("temperature2"), FactMetaData::valueTypeDouble);
    Fact _temperature3Fact = Fact(0, QStringLiteral("temperature3"), FactMetaData::valueTypeDouble);
};
