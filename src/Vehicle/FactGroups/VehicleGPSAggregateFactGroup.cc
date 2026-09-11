/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleGPSAggregateFactGroup.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <array>

#include "QGCLoggingCategory.h"
#include "VehicleGPSFactGroup.h"

VehicleGPSAggregateFactGroup::VehicleGPSAggregateFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/GPS/Integrity/GPSFact.json", parent)
{
    _addFact(&_spoofingStateFact);
    _addFact(&_jammingStateFact);
    _addFact(&_authenticationStateFact);
    _addFact(&_isStaleFact);

    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _isStaleFact.setRawValue(true);
}

void VehicleGPSAggregateFactGroup::bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    _clearConnections();
    _gps1 = gps1 ? gps1->integrity()->store() : nullptr;
    _gps2 = gps2 ? gps2->integrity()->store() : nullptr;
    for (auto* store : {_gps1.data(), _gps2.data()}) {
        if (store) {
            _connections << connect(store, &GPSIntegrityStore::observationChanged, this,
                                    &VehicleGPSAggregateFactGroup::_updateAggregates);
            _connections << connect(store, &QObject::destroyed, this, &VehicleGPSAggregateFactGroup::_updateAggregates);
        }
    }
    _updateAggregates();
}

void VehicleGPSAggregateFactGroup::_updateAggregates()
{
    const auto first = _gps1 ? _gps1->observation() : GPSIntegrityObservation();
    const auto second = _gps2 ? _gps2->observation() : GPSIntegrityObservation();
    const auto spoofing = _mergeWorst(first.spoofingState.value_or(-1), second.spoofingState.value_or(-1));
    const auto jamming = _mergeWorst(first.jammingState.value_or(-1), second.jammingState.value_or(-1));
    const auto authentication =
        _mergeAuthentication(first.authenticationState.value_or(-1), second.authenticationState.value_or(-1));
    const bool stale = !((_gps1 && _gps1->available()) || (_gps2 && _gps2->available()));
    const QPointer<VehicleGPSAggregateFactGroup> guard(this);
    const auto revision = ++_revision;
    const std::array<std::pair<Fact*, QVariant>, 4> values = {{
        {&_spoofingStateFact, spoofing == -1 ? 255 : spoofing},
        {&_jammingStateFact, jamming == -1 ? 255 : jamming},
        {&_authenticationStateFact, authentication == -1 ? 255 : authentication},
        {&_isStaleFact, stale},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || revision != _revision) {
            return;
        }
        fact->setRawValue(value);
    }
}

void VehicleGPSAggregateFactGroup::_clearConnections()
{
    for (const auto& c : _connections) {
        QObject::disconnect(c);
    }
    _connections.clear();
}

int VehicleGPSAggregateFactGroup::_mergeWorst(int a, int b)
{
    return (std::max) (a, b);
}

int VehicleGPSAggregateFactGroup::_mergeAuthentication(int a, int b)
{
    // Priority: Unknown < Disabled < Initializing < OK < Error
    auto getWeight = [](int val) {
        switch (val) {
            case AUTH_INVALID:
                return -1;
            case AUTH_UNKNOWN:
                return 0;  // lowest priority)
            case AUTH_DISABLED:
                return 1;
            case AUTH_INITIALIZING:
                return 2;
            case AUTH_OK:
                return 3;
            case AUTH_ERROR:
                return 4;  // highest priority
            default:
                return -1;
        }
    };

    return (getWeight(a) >= getWeight(b)) ? a : b;
}

void VehicleGPSAggregateFactGroup::updateFromGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    bindToGps(gps1, gps2);
}
