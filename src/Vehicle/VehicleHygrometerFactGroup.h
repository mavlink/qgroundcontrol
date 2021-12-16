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

class VehicleHygrometerFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleHygrometerFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* hygroID            READ hygroID            CONSTANT)
    Q_PROPERTY(Fact* hygroTemp          READ hygroTemp          CONSTANT)
    Q_PROPERTY(Fact* hygroHumi          READ hygroHumi          CONSTANT)

    Fact* hygroID                           () { return &_hygroIDFact; }
    Fact* hygroTemp                         () { return &_hygroTempFact; }
    Fact* hygroHumi                         () { return &_hygroHumiFact; }

    // Overrides from FactGroup
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _hygroIDFactName;
    static const char* _hygroTempFactName;
    static const char* _hygroHumiFactName;

protected:
    void _handleHygrometerSensor        (mavlink_message_t& message);

    Fact _hygroTempFact;
    Fact _hygroHumiFact;
    Fact _hygroIDFact;
};
