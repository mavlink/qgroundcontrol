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

class VehicleWindFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleWindFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* direction      READ direction      CONSTANT)
    Q_PROPERTY(Fact* speed          READ speed          CONSTANT)
    Q_PROPERTY(Fact* verticalSpeed  READ verticalSpeed  CONSTANT)

    Fact* direction     () { return &_directionFact; }
    Fact* speed         () { return &_speedFact; }
    Fact* verticalSpeed () { return &_verticalSpeedFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _directionFactName;
    static const char* _speedFactName;
    static const char* _verticalSpeedFactName;

private:
    void _handleHighLatency (mavlink_message_t& message);
    void _handleHighLatency2(mavlink_message_t& message);
    void _handleWindCov     (mavlink_message_t& message);
#if !defined(NO_ARDUPILOT_DIALECT)
    void _handleWind        (mavlink_message_t& message);
#endif

    Fact        _directionFact;
    Fact        _speedFact;
    Fact        _verticalSpeedFact;
};
