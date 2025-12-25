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
#include "VehicleGPS2FactGroup.h"
#include "QGCLoggingCategory.h"
#include <QtMath>

VehicleGPSAggregateFactGroup::VehicleGPSAggregateFactGroup(QObject *parent)
    : FactGroup(0, parent)
    , _spoofingStateFact      (0, "spoofingState",       FactMetaData::valueTypeUint8, this)
    , _jammingStateFact       (0, "jammingState",        FactMetaData::valueTypeUint8, this)
    , _authenticationStateFact(0, "authenticationState", FactMetaData::valueTypeUint8, this)
    , _isStaleFact            (0, "isStale",             FactMetaData::valueTypeBool, this)
{
    _spoofingStateFact.setEnumInfo(
        QStringList({ "Unknown", "Not spoofed", "Mitigated", "Ongoing" }),
        QVariantList({ 0, 1, 2, 3 })
    );

    _jammingStateFact.setEnumInfo(
        QStringList({ "Unknown", "Not jammed", "Mitigated", "Ongoing" }),
        QVariantList({ 0, 1, 2, 3 })
    );

    _authenticationStateFact.setEnumInfo(
        QStringList({ "Unknown", "Initializing", "Error", "Ok", "Disabled" }),
        QVariantList({ 0, 1, 2, 3, 4 })
    );

    _addFact(&_spoofingStateFact);
    _addFact(&_jammingStateFact);
    _addFact(&_authenticationStateFact);
    _addFact(&_isStaleFact);

    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _isStaleFact.setRawValue(true);

    _staleTimer.setSingleShot(true);
    _staleTimer.setInterval(5000);
    connect(&_staleTimer, &QTimer::timeout, this, &VehicleGPSAggregateFactGroup::_onStaleTimeout);
}

void VehicleGPSAggregateFactGroup::bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    _clearConnections();
    _gps1 = gps1;
    _gps2 = gps2;

    auto connectFact = [this](Fact* fact, const QString& name) {
        if (fact) {
            Q_UNUSED(name);
            _connections << connect(fact, &Fact::valueChanged, this, &VehicleGPSAggregateFactGroup::_onIntegrityUpdated);
        }
    };

    if (_gps1) {
        connectFact(_gps1->spoofingState(), "GPS1 spoofingState");
        connectFact(_gps1->jammingState(), "GPS1 jammingState");
        connectFact(_gps1->authenticationState(), "GPS1 authenticationState");
    }
    if (_gps2) {
        connectFact(_gps2->spoofingState(), "GPS2 spoofingState");
        connectFact(_gps2->jammingState(), "GPS2 jammingState");
        connectFact(_gps2->authenticationState(), "GPS2 authenticationState");
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
    _updateAggregates();
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
        return -1;
    }
    const QVariant v = fact->rawValue();
    if (!v.isValid()) {
        return -1;
    }
    bool ok = false;
    const int val = v.toInt(&ok);
    if (!ok) {
        return -1;
    }
    return (val == 255) ? -1 : val;
}

int VehicleGPSAggregateFactGroup::_mergeWorst(int a, int b)
{
    return qMax(a, b);
}

int VehicleGPSAggregateFactGroup::_mergeAuthentication(int a, int b)
{
    //Priority: 0 (Unknown) < 4 (Disabled) < 1 (Initializing) < 3 (OK) < 2 (Error)
    auto getWeight = [](int val) {
        switch (val) {
        case -1: return -1;
        case 0: return 0;
        case 4: return 1;
        case 1: return 2;
        case 3: return 3;
        case 2: return 4;
        default: return -1;
        }
    };

    return (getWeight(a) >= getWeight(b)) ? a : b;
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

    _spoofingStateFact.setRawValue(spoofMerged == -1 ? 255 : spoofMerged);
    _jammingStateFact.setRawValue(jamMerged == -1 ? 255 : jamMerged);
    _authenticationStateFact.setRawValue(authMerged == -1 ? 255 : authMerged);
}
