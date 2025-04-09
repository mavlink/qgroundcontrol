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

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *lat                READ lat                CONSTANT)
    Q_PROPERTY(Fact *lon                READ lon                CONSTANT)
    Q_PROPERTY(Fact *mgrs               READ mgrs               CONSTANT)
    Q_PROPERTY(Fact *hdop               READ hdop               CONSTANT)
    Q_PROPERTY(Fact *vdop               READ vdop               CONSTANT)
    Q_PROPERTY(Fact *courseOverGround   READ courseOverGround   CONSTANT)
    Q_PROPERTY(Fact *count              READ count              CONSTANT)
    Q_PROPERTY(Fact *lock               READ lock               CONSTANT)

public:
    explicit VehicleGPSFactGroup(QObject *parent = nullptr);

    Fact *lat() { return &_latFact; }
    Fact *lon() { return &_lonFact; }
    Fact *mgrs() { return &_mgrsFact; }
    Fact *hdop() { return &_hdopFact; }
    Fact *vdop() { return &_vdopFact; }
    Fact *courseOverGround() { return &_courseOverGroundFact; }
    Fact *count() { return &_countFact; }
    Fact *lock() { return &_lockFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

protected:
    void _handleGpsRawInt(const mavlink_message_t &message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);

    Fact _latFact = Fact(0, QStringLiteral("lat"), FactMetaData::valueTypeDouble);
    Fact _lonFact = Fact(0, QStringLiteral("lon"), FactMetaData::valueTypeDouble);
    Fact _mgrsFact = Fact(0, QStringLiteral("mgrs"), FactMetaData::valueTypeString);
    Fact _hdopFact = Fact(0, QStringLiteral("hdop"), FactMetaData::valueTypeDouble);
    Fact _vdopFact = Fact(0, QStringLiteral("vdop"), FactMetaData::valueTypeDouble);
    Fact _courseOverGroundFact = Fact(0, QStringLiteral("courseOverGround"), FactMetaData::valueTypeDouble);
    Fact _countFact = Fact(0, QStringLiteral("count"), FactMetaData::valueTypeInt32);
    Fact _lockFact = Fact(0, QStringLiteral("lock"), FactMetaData::valueTypeInt32);
};
