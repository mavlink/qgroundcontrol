/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleGPSAggregateFactGroup.h"

#include <algorithm>

#include "MonotonicClock.h"
#include "QtRuntimeScheduler.h"
#include "VehicleGPSFactGroup.h"

VehicleGPSAggregateFactGroup::VehicleGPSAggregateFactGroup(QObject* parent, RuntimeScheduler* scheduler)
    : FactGroup(1000, ":/json/Vehicle/GPSFact.json", parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _expiryTask(_scheduler, this)
{
    _addFact(&_spoofingStateFact);
    _addFact(&_jammingStateFact);
    _addFact(&_authenticationStateFact);
    _addFact(&_isStaleFact);

    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _isStaleFact.setRawValue(true);

    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        _updateAggregates();
    });
}

VehicleGPSAggregateFactGroup::~VehicleGPSAggregateFactGroup()
{
    _clearConnections();
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
}

void VehicleGPSAggregateFactGroup::bindToGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    _clearConnections();
    _gps1 = gps1;
    _gps2 = gps2;

    const auto update = [this, revision = _bindingRevision]() {
        if (revision == _bindingRevision) {
            _updateAggregates();
        }
    };
    for (auto* gps : {gps1, gps2}) {
        if (gps) {
            _connections << connect(gps, &VehicleGPSFactGroup::gnssIntegrityReceived, this, update);
            _connections << connect(gps, &QObject::destroyed, this, update);
        }
    }
    _updateAggregates();
}

void VehicleGPSAggregateFactGroup::updateFromGps(VehicleGPSFactGroup* gps1, VehicleGPSFactGroup* gps2)
{
    bindToGps(gps1, gps2);
}

void VehicleGPSAggregateFactGroup::_clearConnections()
{
    ++_bindingRevision;
    _expiryTask.cancel();
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
    // Priority: Unknown < Disabled < Initializing < OK < Error
    auto getWeight = [](int val) {
        switch (val) {
        case AUTH_INVALID:      return -1;
        case AUTH_UNKNOWN:      return 0;   // lowest priority)
        case AUTH_DISABLED:     return 1;
        case AUTH_INITIALIZING: return 2;
        case AUTH_OK:           return 3;
        case AUTH_ERROR:        return 4;   // highest priority
        default:                return -1;
        }
    };

    return (getWeight(a) >= getWeight(b)) ? a : b;
}

void VehicleGPSAggregateFactGroup::_updateAggregates()
{
    const QPointer<VehicleGPSAggregateFactGroup> guard(this);
    const quint64 revision = ++_updateRevision;
    _expiryTask.cancel();
    const quint64 nowUs = _scheduler ? _scheduler->nowUs() : 0;
    const auto remaining1 =
        MonotonicClock::remaining(_gps1 ? _gps1->gnssIntegrityTimestampUs() : 0, nowUs, GNSS_INTEGRITY_STALE_TIMEOUT);
    const auto remaining2 =
        MonotonicClock::remaining(_gps2 ? _gps2->gnssIntegrityTimestampUs() : 0, nowUs, GNSS_INTEGRITY_STALE_TIMEOUT);
    auto* gps1 = remaining1.count() > 0 ? _gps1.data() : nullptr;
    auto* gps2 = remaining2.count() > 0 ? _gps2.data() : nullptr;
    const bool available = gps1 || gps2;
    if (available) {
        const auto nextExpiry = gps1 && gps2 ? std::min(remaining1, remaining2) : std::max(remaining1, remaining2);
        _expiryTask.schedule(nextExpiry, [this]() { _updateAggregates(); });
    }

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
    if (!guard || revision != _updateRevision) {
        return;
    }
    _jammingStateFact.setRawValue(jamMerged == -1 ? 255 : jamMerged);
    if (!guard || revision != _updateRevision) {
        return;
    }
    _authenticationStateFact.setRawValue(authMerged == -1 ? 255 : authMerged);
    if (!guard || revision != _updateRevision) {
        return;
    }
    _isStaleFact.setRawValue(!available);
    if (!guard || revision != _updateRevision) {
        return;
    }
    _setTelemetryAvailable(available);
}
