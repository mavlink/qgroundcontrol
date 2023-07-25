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

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleGPSFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* lat                READ lat                CONSTANT)
    Q_PROPERTY(Fact* lon                READ lon                CONSTANT)
    Q_PROPERTY(Fact* mgrs               READ mgrs               CONSTANT)
    Q_PROPERTY(Fact* hdop               READ hdop               CONSTANT)
    Q_PROPERTY(Fact* vdop               READ vdop               CONSTANT)
    Q_PROPERTY(Fact* courseOverGround   READ courseOverGround   CONSTANT)
    Q_PROPERTY(Fact* count              READ count              CONSTANT)
    Q_PROPERTY(Fact* lock               READ lock               CONSTANT)

    Fact* lat               () { return &_latFact; }
    Fact* lon               () { return &_lonFact; }
    Fact* mgrs              () { return &_mgrsFact; }
    Fact* hdop              () { return &_hdopFact; }
    Fact* vdop              () { return &_vdopFact; }
    Fact* courseOverGround  () { return &_courseOverGroundFact; }
    Fact* count             () { return &_countFact; }
    Fact* lock              () { return &_lockFact; }

    // Overrides from FactGroup
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _latFactName;
    static const char* _lonFactName;
    static const char* _mgrsFactName;
    static const char* _hdopFactName;
    static const char* _vdopFactName;
    static const char* _courseOverGroundFactName;
    static const char* _countFactName;
    static const char* _lockFactName;

protected:
    void _handleGpsRawInt   (mavlink_message_t& message);
    void _handleHighLatency (mavlink_message_t& message);
    void _handleHighLatency2(mavlink_message_t& message);

    Fact _latFact;
    Fact _lonFact;
    Fact _mgrsFact;
    Fact _hdopFact;
    Fact _vdopFact;
    Fact _courseOverGroundFact;
    Fact _countFact;
    Fact _lockFact;
};
