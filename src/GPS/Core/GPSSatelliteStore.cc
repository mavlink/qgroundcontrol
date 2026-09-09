#include "GPSSatelliteStore.h"

#include <algorithm>
#include <limits>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSatelliteStoreLog, "GPS.Core.GPSSatelliteStore")

GPSSatelliteStore::GPSSatelliteStore(QObject* parent, int freshnessTimeoutMs)
    : QObject(parent)
    , _expiryTimer(this)
    , _freshnessTimeoutMs(std::max(1, freshnessTimeoutMs))
{
    qCDebug(GPSSatelliteStoreLog) << this;
    _expiryTimer.setSingleShot(true);
    _expiryTimer.setTimerType(Qt::PreciseTimer);
    connect(&_expiryTimer, &QTimer::timeout, this, &GPSSatelliteStore::_publish);
}

GPSSatelliteStore::~GPSSatelliteStore()
{
    qCDebug(GPSSatelliteStoreLog) << this;
}

void GPSSatelliteStore::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_observation.sourceId == sourceId && _observation.sessionId == sessionId) {
        return;
    }
    _constellations.clear();
    _clearedThroughUs = 0;
    _observation = {};
    _observation.sourceId = sourceId;
    _observation.sessionId = sessionId;
    _publish();
}

void GPSSatelliteStore::reset()
{
    _constellations.clear();
    _clearedThroughUs = 0;
    _observation = {};
    _publish();
}

void GPSSatelliteStore::clear()
{
    _clearedThroughUs = GPSObservation::monotonicNowUs();
    _constellations.clear();
    _publish();
}

void GPSSatelliteStore::setFreshnessTimeoutMs(int timeoutMs)
{
    if (_freshnessTimeoutMs != std::max(1, timeoutMs)) {
        _freshnessTimeoutMs = std::max(1, timeoutMs);
        _publish();
    }
}

bool GPSSatelliteStore::_accept(quint64 receipt, quint64 current, quint64& retired, quint64 nowUs) const
{
    if (!receipt || receipt <= _clearedThroughUs || receipt <= retired || receipt < current || receipt > nowUs) {
        return false;
    }
    if (nowUs - receipt >= static_cast<quint64>(_freshnessTimeoutMs) * 1000) {
        retired = std::max(retired, receipt);
        return false;
    }
    return true;
}

void GPSSatelliteStore::updateObservation(const GPSSatelliteObservation& observation)
{
    if (_observation.sourceId.isEmpty() || observation.sessionId != _observation.sessionId ||
        (!observation.sourceId.isEmpty() && observation.sourceId != _observation.sourceId)) {
        return;
    }
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    _expire(nowUs);
    auto reports = observation.provenance;
    if (reports.isEmpty()) {
        // Native reports have one receipt for the full view/use snapshot.
        std::map<GPSSatellite::Constellation, int> counts;
        for (const auto& satellite : observation.satellites) {
            counts.try_emplace(satellite.constellation, 0);
            counts[satellite.constellation] += satellite.used.value_or(false) ? 1 : 0;
        }
        if (counts.empty()) {
            counts[GPSSatellite::Constellation::Unknown] = 0;
        }
        for (const auto& [constellation, count] : counts) {
            const bool known = std::all_of(
                observation.satellites.cbegin(), observation.satellites.cend(), [constellation](const auto& satellite) {
                    return satellite.constellation != constellation || satellite.used.has_value();
                });
            reports.append({constellation, observation.monotonicTimestampUs,
                            known ? observation.monotonicTimestampUs : 0,
                            known ? std::optional<int>(count) : std::nullopt});
        }
    }
    for (const auto& report : reports) {
        if (report.constellation < GPSSatellite::Constellation::Unknown ||
            report.constellation > GPSSatellite::Constellation::NavIC) {
            continue;
        }
        auto& state = _constellations[report.constellation];
        if (_accept(report.inViewTimestampUs, state.viewReceiptUs, state.viewRetiredThroughUs, nowUs)) {
            state.viewReceiptUs = report.inViewTimestampUs;
            state.satellites.clear();
            for (const auto& satellite : observation.satellites) {
                if (satellite.constellation == report.constellation) {
                    state.satellites.append(satellite);
                }
            }
        }
        if (report.satellitesUsed && *report.satellitesUsed >= 0 &&
            _accept(report.inUseTimestampUs, state.useReceiptUs, state.useRetiredThroughUs, nowUs)) {
            state.useReceiptUs = report.inUseTimestampUs;
            state.usedCount = report.satellitesUsed;
            state.used.clear();
            for (const auto& satellite : observation.satellites) {
                if (satellite.constellation == report.constellation && satellite.used) {
                    state.used[{satellite.id, satellite.prn}] = *satellite.used;
                }
            }
        }
    }
    _publish();
}

void GPSSatelliteStore::_expire(quint64 nowUs)
{
    const quint64 lifetimeUs = static_cast<quint64>(_freshnessTimeoutMs) * 1000;
    for (auto& [constellation, state] : _constellations) {
        if (state.viewReceiptUs && nowUs - state.viewReceiptUs >= lifetimeUs) {
            state.viewRetiredThroughUs = std::max(state.viewRetiredThroughUs, state.viewReceiptUs);
            state.viewReceiptUs = 0;
            state.satellites.clear();
        }
        if (state.useReceiptUs && nowUs - state.useReceiptUs >= lifetimeUs) {
            state.useRetiredThroughUs = std::max(state.useRetiredThroughUs, state.useReceiptUs);
            state.useReceiptUs = 0;
            state.usedCount.reset();
            state.used.clear();
        }
    }
}

void GPSSatelliteStore::_publish()
{
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    _expire(nowUs);
    _observation.satellites.clear();
    _observation.provenance.clear();
    _observation.monotonicTimestampUs = 0;
    quint64 nextExpiryUs = std::numeric_limits<quint64>::max();
    for (const auto& [constellation, state] : _constellations) {
        if (!state.viewReceiptUs && !state.useReceiptUs) {
            continue;
        }
        _observation.provenance.append({constellation, state.viewReceiptUs, state.useReceiptUs, state.usedCount});
        for (auto satellite : state.satellites) {
            const auto used = state.used.find({satellite.id, satellite.prn});
            satellite.used =
                state.useReceiptUs && used != state.used.cend() ? std::optional<bool>(used->second) : std::nullopt;
            _observation.satellites.append(satellite);
        }
        for (const quint64 receipt : {state.viewReceiptUs, state.useReceiptUs}) {
            if (receipt) {
                _observation.monotonicTimestampUs = std::max(_observation.monotonicTimestampUs, receipt);
                nextExpiryUs = std::min(nextExpiryUs, receipt + static_cast<quint64>(_freshnessTimeoutMs) * 1000);
            }
        }
    }
    _expiryTimer.stop();
    if (nextExpiryUs != std::numeric_limits<quint64>::max()) {
        const quint64 remainingMs = (nextExpiryUs - nowUs + 999) / 1000;
        _expiryTimer.start(static_cast<int>(std::min<quint64>(remainingMs, std::numeric_limits<int>::max())));
    }
    _observation.revision = ++_revision;
    const GPSSatelliteObservation snapshot = _observation;
    emit observationChanged(snapshot);
}
