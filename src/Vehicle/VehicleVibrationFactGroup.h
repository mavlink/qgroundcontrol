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

class VehicleVibrationFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleVibrationFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* xAxis      READ xAxis      CONSTANT)
    Q_PROPERTY(Fact* yAxis      READ yAxis      CONSTANT)
    Q_PROPERTY(Fact* zAxis      READ zAxis      CONSTANT)
    Q_PROPERTY(Fact* clipCount1 READ clipCount1 CONSTANT)
    Q_PROPERTY(Fact* clipCount2 READ clipCount2 CONSTANT)
    Q_PROPERTY(Fact* clipCount3 READ clipCount3 CONSTANT)

    Fact* xAxis         () { return &_xAxisFact; }
    Fact* yAxis         () { return &_yAxisFact; }
    Fact* zAxis         () { return &_zAxisFact; }
    Fact* clipCount1    () { return &_clipCount1Fact; }
    Fact* clipCount2    () { return &_clipCount2Fact; }
    Fact* clipCount3    () { return &_clipCount3Fact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static constexpr const char* _xAxisFactName      = "xAxis";
    static constexpr const char* _yAxisFactName      = "yAxis";
    static constexpr const char* _zAxisFactName      = "zAxis";
    static constexpr const char* _clipCount1FactName = "clipCount1";
    static constexpr const char* _clipCount2FactName = "clipCount2";
    static constexpr const char* _clipCount3FactName = "clipCount3";

private:
    Fact        _xAxisFact;
    Fact        _yAxisFact;
    Fact        _zAxisFact;
    Fact        _clipCount1Fact;
    Fact        _clipCount2Fact;
    Fact        _clipCount3Fact;
};
