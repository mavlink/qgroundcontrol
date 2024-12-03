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
#include "QGCMAVLink.h"

class VehicleRPMFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleRPMFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* rpm1 READ rpm1 CONSTANT)
    Q_PROPERTY(Fact* rpm2 READ rpm2 CONSTANT)
    Q_PROPERTY(Fact* rpm3 READ rpm3 CONSTANT)
    Q_PROPERTY(Fact* rpm4 READ rpm4 CONSTANT)

    Fact* rpm1 () { return &_rpm1Fact; }
    Fact* rpm2 () { return &_rpm2Fact; }
    Fact* rpm3 () { return &_rpm3Fact; }
    Fact* rpm4 () { return &_rpm4Fact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _rpm1FactName;
    static const char* _rpm2FactName;
    static const char* _rpm3FactName;
    static const char* _rpm4FactName;

private:
    Fact _rpm1Fact;
    Fact _rpm2Fact;
    Fact _rpm3Fact;
    Fact _rpm4Fact;
};
