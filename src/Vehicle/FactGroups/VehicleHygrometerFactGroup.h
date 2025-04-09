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

class VehicleHygrometerFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *hygroID    READ hygroID    CONSTANT)
    Q_PROPERTY(Fact *hygroTemp  READ hygroTemp  CONSTANT)
    Q_PROPERTY(Fact *hygroHumi  READ hygroHumi  CONSTANT)

public:
    explicit VehicleHygrometerFactGroup(QObject *parent = nullptr);

    Fact *hygroID() { return &_hygroIDFact; }
    Fact *hygroTemp() { return &_hygroTempFact; }
    Fact *hygroHumi() { return &_hygroHumiFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

protected:
    void _handleHygrometerSensor(const mavlink_message_t &message);

    Fact _hygroTempFact = Fact(0, QStringLiteral("temperature"), FactMetaData::valueTypeDouble);
    Fact _hygroHumiFact = Fact(0, QStringLiteral("humidity"), FactMetaData::valueTypeDouble);
    Fact _hygroIDFact = Fact(0, QStringLiteral("hygrometerid"), FactMetaData::valueTypeUint16);
};
