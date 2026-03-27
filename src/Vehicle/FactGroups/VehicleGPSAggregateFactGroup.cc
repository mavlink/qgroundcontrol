/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleGPSAggregateFactGroup.h"
#include "VehicleGPSFactGroup.h"
#include "QGCLoggingCategory.h"
#include <QtMath>

VehicleGPSAggregateFactGroup::VehicleGPSAggregateFactGroup(QObject *parent)
    : FactGroup(1000, ":/json/Vehicle/GPSAggregateFact.json", parent)
{
    _addFact(&_spoofingStateFact);
    _addFact(&_jammingStateFact);
    _addFact(&_authenticationStateFact);
    _addFact(&_isStaleFact);

    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _isStaleFact.setRawValue(true);

    _staleTimer.setSingleShot(true);
    _staleTimer.setInterval(GNSS_INTEGRITY_STALE_TIMEOUT_MS);
    connect(&_staleTimer, &QTimer::timeout, this, &VehicleGPSAggregateFactGroup::_onStaleTimeout);
}

void VehicleGPSAggregateFactGroup::bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    _clearConnections();
    _gps1 = gps1;
    _gps2 = gps2;

    if (_gps1) {
        _connections << connect(_gps1, &VehicleGPSFactGroup::gnssIntegrityReceived, this, &VehicleGPSAggregateFactGroup::_onIntegrityUpdated);
    }
    if (_gps2) {
        _connections << connect(_gps2, &VehicleGPSFactGroup::gnssIntegrityReceived, this, &VehicleGPSAggregateFactGroup::_onIntegrityUpdated);
    }
    _updateAggregates();
}

void VehicleGPSAggregateFactGroup::_updateAggregates()
{
    updateFromGps(_gps1, _gps2);
}

void VehicleGPSAggregateFactGroup::_onIntegrityUpdated()
{
    _isStaleFact.setRawValue(false);
    _staleTimer.start();
    _updateAggregates();
}

void VehicleGPSAggregateFactGroup::_onStaleTimeout()
{
    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _isStaleFact.setRawValue(true);
}

void VehicleGPSAggregateFactGroup::_clearConnections()
{
    for (const auto& c : _connections) {
        QObject::disconnect(c);
    }
    _connections.clear();
}

int VehicleGPSAggregateFactGroup::_valueOrInvalid(Fact* fact)
{
    if (!fact) {
        return 255;
    }
    const QVariant v = fact->rawValue();
    if (!v.isValid()) {
        return 255;
    }
    bool ok = false;
    const int val = v.toInt(&ok);
    return ok ? val : 255;
}

int VehicleGPSAggregateFactGroup::_mergeWorst(int a, int b)
{
    // 255 is the invalid sentinel; prefer a valid reading over none
    const bool aValid = (a != 255);
    const bool bValid = (b != 255);
    if (!aValid && !bValid) return 255;
    if (!aValid) return b;
    if (!bValid) return a;
    return qMax(a, b);
}

int VehicleGPSAggregateFactGroup::_mergeAuthentication(int a, int b)
{
    using AS = VehicleGPSFactGroup::AuthenticationState;
    // Priority: Unknown < Disabled < Initializing < OK < Error
    // Invalid (255) means no data — prefer any valid reading over none
    auto getWeight = [](int val) -> int {
        switch (static_cast<AS>(val)) {
        case AS::AuthUnknown:      return 0;   // lowest priority
        case AS::AuthDisabled:     return 1;
        case AS::AuthInitializing: return 2;
        case AS::AuthOk:           return 3;
        case AS::AuthError:        return 4;   // highest priority
        case AS::AuthInvalid:      return -1;
        default:                   return -1;
        }
    };

    const int wa = getWeight(a);
    const int wb = getWeight(b);
    if (wa < 0 && wb < 0) return static_cast<int>(AS::AuthInvalid);
    if (wa < 0) return b;
    if (wb < 0) return a;
    return (wa >= wb) ? a : b;
}

void VehicleGPSAggregateFactGroup::updateFromGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    const int spoof1 = _valueOrInvalid(gps1 ? gps1->spoofingState()       : nullptr);
    const int spoof2 = _valueOrInvalid(gps2 ? gps2->spoofingState()       : nullptr);
    const int jam1   = _valueOrInvalid(gps1 ? gps1->jammingState()        : nullptr);
    const int jam2   = _valueOrInvalid(gps2 ? gps2->jammingState()        : nullptr);
    const int auth1  = _valueOrInvalid(gps1 ? gps1->authenticationState() : nullptr);
    const int auth2  = _valueOrInvalid(gps2 ? gps2->authenticationState() : nullptr);

    const int spoofMerged = _mergeWorst(spoof1, spoof2);
    const int jamMerged   = _mergeWorst(jam1,   jam2);
    const int authMerged  = _mergeAuthentication(auth1, auth2);

    _spoofingStateFact.setRawValue(spoofMerged);
    _jammingStateFact.setRawValue(jamMerged);
    _authenticationStateFact.setRawValue(authMerged);
}
