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

class VehicleVibrationFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *xAxis      READ xAxis      CONSTANT)
    Q_PROPERTY(Fact *yAxis      READ yAxis      CONSTANT)
    Q_PROPERTY(Fact *zAxis      READ zAxis      CONSTANT)
    Q_PROPERTY(Fact *clipCount1 READ clipCount1 CONSTANT)
    Q_PROPERTY(Fact *clipCount2 READ clipCount2 CONSTANT)
    Q_PROPERTY(Fact *clipCount3 READ clipCount3 CONSTANT)

public:
    explicit VehicleVibrationFactGroup(QObject *parent = nullptr);

    Fact *xAxis() { return &_xAxisFact; }
    Fact *yAxis() { return &_yAxisFact; }
    Fact *zAxis() { return &_zAxisFact; }
    Fact *clipCount1() { return &_clipCount1Fact; }
    Fact *clipCount2() { return &_clipCount2Fact; }
    Fact *clipCount3() { return &_clipCount3Fact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _xAxisFact = Fact(0, QStringLiteral("xAxis"), FactMetaData::valueTypeDouble);
    Fact _yAxisFact = Fact(0, QStringLiteral("yAxis"), FactMetaData::valueTypeDouble);
    Fact _zAxisFact = Fact(0, QStringLiteral("zAxis"), FactMetaData::valueTypeDouble);
    Fact _clipCount1Fact = Fact(0, QStringLiteral("clipCount1"), FactMetaData::valueTypeUint32);
    Fact _clipCount2Fact = Fact(0, QStringLiteral("clipCount2"), FactMetaData::valueTypeUint32);
    Fact _clipCount3Fact = Fact(0, QStringLiteral("clipCount3"), FactMetaData::valueTypeUint32);
};
