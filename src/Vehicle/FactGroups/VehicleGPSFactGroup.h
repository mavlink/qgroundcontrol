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

    Q_PROPERTY(Fact* lat                    READ lat                    CONSTANT)
    Q_PROPERTY(Fact* lon                    READ lon                    CONSTANT)
    Q_PROPERTY(Fact* mgrs                   READ mgrs                   CONSTANT)
    Q_PROPERTY(Fact* hdop                   READ hdop                   CONSTANT)
    Q_PROPERTY(Fact* vdop                   READ vdop                   CONSTANT)
    Q_PROPERTY(Fact* courseOverGround       READ courseOverGround       CONSTANT)
    Q_PROPERTY(Fact* count                  READ count                  CONSTANT)
    Q_PROPERTY(Fact* lock                   READ lock                   CONSTANT)
    Q_PROPERTY(Fact* systemErrors           READ systemErrors           CONSTANT)
    Q_PROPERTY(Fact* spoofingState          READ spoofingState          CONSTANT)
    Q_PROPERTY(Fact* authenticationState    READ authenticationState    CONSTANT)

    Fact* lat                   () { return &_latFact; }
    Fact* lon                   () { return &_lonFact; }
    Fact* mgrs                  () { return &_mgrsFact; }
    Fact* hdop                  () { return &_hdopFact; }
    Fact* vdop                  () { return &_vdopFact; }
    Fact* courseOverGround      () { return &_courseOverGroundFact; }
    Fact* count                 () { return &_countFact; }
    Fact* lock                  () { return &_lockFact; }
    Fact* systemErrors          () { return &_systemErrorsFact; }
    Fact* spoofingState         () { return &_spoofingStateFact; }
    Fact* authenticationState   () { return &_authenticationStateFact; }

    // Overrides from FactGroup
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

protected:
    void _handleGpsRawInt    (mavlink_message_t& message);
    void _handleHighLatency  (mavlink_message_t& message);
    void _handleHighLatency2 (mavlink_message_t& message);
    void _handleGnssIntegrity(mavlink_message_t& message);

    const QString _latFactName =                 QStringLiteral("lat");
    const QString _lonFactName =                 QStringLiteral("lon");
    const QString _mgrsFactName =                QStringLiteral("mgrs");
    const QString _hdopFactName =                QStringLiteral("hdop");
    const QString _vdopFactName =                QStringLiteral("vdop");
    const QString _courseOverGroundFactName =    QStringLiteral("courseOverGround");
    const QString _countFactName =               QStringLiteral("count");
    const QString _lockFactName =                QStringLiteral("lock");
    const QString _systemErrorsFactName =        QStringLiteral("systemErrors");
    const QString _spoofingStateFactName =       QStringLiteral("spoofingState");
    const QString _authenticationStateFactName = QStringLiteral("authenticationState");

    Fact _latFact;
    Fact _lonFact;
    Fact _mgrsFact;
    Fact _hdopFact;
    Fact _vdopFact;
    Fact _courseOverGroundFact;
    Fact _countFact;
    Fact _lockFact;
    Fact _systemErrorsFact;
    Fact _spoofingStateFact;
    Fact _authenticationStateFact;
};
