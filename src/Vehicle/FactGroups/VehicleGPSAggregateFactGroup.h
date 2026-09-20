/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <chrono>

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include "FactGroup.h"
#include "ScheduledTask.h"

class VehicleGPSFactGroup;

class VehicleGPSAggregateFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* spoofingState       READ spoofingState       CONSTANT)
    Q_PROPERTY(Fact* jammingState        READ jammingState        CONSTANT)
    Q_PROPERTY(Fact* authenticationState READ authenticationState CONSTANT)
    Q_PROPERTY(Fact* isStale             READ isStale             CONSTANT)
public:
    enum AuthState {
        AUTH_UNKNOWN = 0,
        AUTH_INITIALIZING = 1,
        AUTH_ERROR = 2,
        AUTH_OK = 3,
        AUTH_DISABLED = 4,
        AUTH_INVALID = -1
    };

    explicit VehicleGPSAggregateFactGroup(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~VehicleGPSAggregateFactGroup() override;

    Fact* spoofingState()       { return &_spoofingStateFact; }
    Fact* jammingState()        { return &_jammingStateFact; }
    Fact* authenticationState() { return &_authenticationStateFact; }
    Fact* isStale()             { return &_isStaleFact; }

    /// Bind and refresh without extending either receiver's original integrity lifetime.
    void updateFromGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2);
    void bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2);

private:
    void _updateAggregates();

    static constexpr auto GNSS_INTEGRITY_STALE_TIMEOUT = std::chrono::seconds(5);

    static int  _mergeWorst(int a, int b);
    static int  _mergeAuthentication(int a, int b);
    static int  _valueOrInvalid(Fact* fact);
    void _clearConnections();

    QPointer<VehicleGPSFactGroup> _gps1;
    QPointer<VehicleGPSFactGroup> _gps2;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _expiryTask;
    quint64 _bindingRevision = 0;
    quint64 _updateRevision = 0;
    QVector<QMetaObject::Connection> _connections;

    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _authenticationStateFact = Fact(0, QStringLiteral("authenticationState"), FactMetaData::valueTypeUint8);
    Fact _isStaleFact = Fact(0, QStringLiteral("isStale"), FactMetaData::valueTypeBool);
};
