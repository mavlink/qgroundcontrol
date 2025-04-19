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

class VehicleWindFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *direction      READ direction      CONSTANT)
    Q_PROPERTY(Fact *speed          READ speed          CONSTANT)
    Q_PROPERTY(Fact *verticalSpeed  READ verticalSpeed  CONSTANT)

public:
    explicit VehicleWindFactGroup(QObject *parent = nullptr);

    Fact *direction() { return &_directionFact; }
    Fact *speed() { return &_speedFact; }
    Fact *verticalSpeed() { return &_verticalSpeedFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);
    void _handleWindCov(const mavlink_message_t &message);
#ifndef QGC_NO_ARDUPILOT_DIALECT
    void _handleWind(const mavlink_message_t &message);
#endif

    Fact _directionFact = Fact(0, QStringLiteral("direction"), FactMetaData::valueTypeDouble);
    Fact _speedFact = Fact(0, QStringLiteral("speed"), FactMetaData::valueTypeDouble);
    Fact _verticalSpeedFact = Fact(0, QStringLiteral("verticalSpeed"), FactMetaData::valueTypeDouble);
};
