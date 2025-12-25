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
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtCore/QMetaObject>

class VehicleGPSFactGroup;

class VehicleGPSAggregateFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* spoofingState       READ spoofingState       CONSTANT)
    Q_PROPERTY(Fact* jammingState        READ jammingState        CONSTANT)
    Q_PROPERTY(Fact* authenticationState READ authenticationState CONSTANT)
    Q_PROPERTY(Fact* isStale             READ isStale             CONSTANT)
public:
    explicit VehicleGPSAggregateFactGroup(QObject *parent = nullptr);

    Fact* spoofingState()       { return &_spoofingStateFact; }
    Fact* jammingState()        { return &_jammingStateFact; }
    Fact* authenticationState() { return &_authenticationStateFact; }
    Fact* isStale()             { return &_isStaleFact; }

    void updateFromGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2);
    void bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2);

private slots:
    void _updateAggregates();
    void _onIntegrityUpdated();
    void _onStaleTimeout();

private:
    static constexpr int GNSS_INTEGRITY_STALE_TIMEOUT_MS = 5000;

    static int  _mergeWorst(int a, int b);
    static int  _mergeAuthentication(int a, int b);
    static int  _valueOrInvalid(Fact* fact);
    void _clearConnections();

    VehicleGPSFactGroup* _gps1 = nullptr;
    VehicleGPSFactGroup* _gps2 = nullptr;
    QTimer _staleTimer;
    QVector<QMetaObject::Connection> _connections;

    Fact _spoofingStateFact = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _jammingStateFact = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _authenticationStateFact = Fact(0, QStringLiteral("authenticationState"), FactMetaData::valueTypeUint8);
    Fact _isStaleFact = Fact(0, QStringLiteral("isStale"), FactMetaData::valueTypeBool);
};