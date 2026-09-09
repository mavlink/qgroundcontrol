#include "GPSSatelliteStore.h"

#include <algorithm>
#include <limits>

#include "GPSQtRuntimeScheduler.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSatelliteStoreLog, "GPS.Core.GPSSatelliteStore")

GPSSatelliteStore::GPSSatelliteStore(QObject* parent, int freshnessTimeoutMs, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _freshnessTimeoutMs(std::max(1, freshnessTimeoutMs))
{
    qCDebug(GPSSatelliteStoreLog) << this;
}

GPSSatelliteStore::~GPSSatelliteStore()
{
    qCDebug(GPSSatelliteStoreLog) << this;
    _scheduler->cancel(_expiryTask);
}

void GPSSatelliteStore::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_observation.sourceId == sourceId && _observation.sessionId == sessionId) {
        return;
    }
    _constellations.clear();
    _clearedThroughUs = 0;
    _fullSnapshotReceiptUs = 0;
    _observation = {};
    _observation.sourceId = sourceId;
    _observation.sessionId = sessionId;
    _publish();
}

void GPSSatelliteStore::reset()
{
    _constellations.clear();
    _clearedThroughUs = 0;
    _fullSnapshotReceiptUs = 0;
    _observation = {};
    _publish();
}

void GPSSatelliteStore::clear()
{
    _clearedThroughUs = _scheduler->nowUs();
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
    const quint64 nowUs = _scheduler->nowUs();
    _expire(nowUs);
    auto reports = observation.provenance;
    if (reports.isEmpty() && observation.satellites.isEmpty() &&
        observation.updateMode == GPSSatelliteObservation::UpdateMode::ConstellationDelta) {
        _publish();
        return;
    }
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
    const bool fullSnapshot = observation.updateMode == GPSSatelliteObservation::UpdateMode::FullSnapshot;
    quint64 fullReceipt = observation.monotonicTimestampUs;
    for (const auto& report : reports) {
        fullReceipt = std::max({fullReceipt, report.inViewTimestampUs, report.inUseTimestampUs});
    }
    if (fullSnapshot) {
        if (!fullReceipt || fullReceipt > nowUs || fullReceipt <= _clearedThroughUs ||
            fullReceipt < _fullSnapshotReceiptUs ||
            nowUs - fullReceipt >= static_cast<quint64>(_freshnessTimeoutMs) * 1000) {
            _publish();
            return;
        }
        _fullSnapshotReceiptUs = fullReceipt;
        for (auto& [constellation, state] : _constellations) {
            const auto report = std::find_if(reports.cbegin(), reports.cend(), [constellation](const auto& value) {
                return value.constellation == constellation;
            });
            if ((report == reports.cend() || !report->inViewTimestampUs) && state.viewReceiptUs <= fullReceipt) {
                state.viewRetiredThroughUs = std::max(state.viewRetiredThroughUs, fullReceipt);
                state.viewReceiptUs = 0;
                state.satellites.clear();
            }
            if ((report == reports.cend() || !report->inUseTimestampUs) && state.useReceiptUs <= fullReceipt) {
                state.useRetiredThroughUs = std::max(state.useRetiredThroughUs, fullReceipt);
                state.useReceiptUs = 0;
                state.usedCount.reset();
                state.used.clear();
                state.usedIds.reset();
            }
        }
    }
    for (const auto& report : reports) {
        if (report.constellation < GPSSatellite::Constellation::Unknown ||
            report.constellation > GPSSatellite::Constellation::NavIC) {
            continue;
        }
        auto& state = _constellations[report.constellation];
        if ((fullSnapshot || report.inViewTimestampUs >= _fullSnapshotReceiptUs) &&
            _accept(report.inViewTimestampUs, state.viewReceiptUs, state.viewRetiredThroughUs, nowUs)) {
            state.viewReceiptUs = report.inViewTimestampUs;
            state.satellites.clear();
            for (const auto& satellite : observation.satellites) {
                if (satellite.constellation == report.constellation) {
                    state.satellites.append(satellite);
                }
            }
        }
        if ((fullSnapshot || report.inUseTimestampUs >= _fullSnapshotReceiptUs) && report.satellitesUsed &&
            *report.satellitesUsed >= 0 &&
            _accept(report.inUseTimestampUs, state.useReceiptUs, state.useRetiredThroughUs, nowUs)) {
            state.useReceiptUs = report.inUseTimestampUs;
            state.usedCount = report.satellitesUsed;
            state.usedIds = report.usedSatelliteIds;
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
            state.usedIds.reset();
        }
    }
}

void GPSSatelliteStore::_publish()
{
    const quint64 nowUs = _scheduler->nowUs();
    _expire(nowUs);
    _observation.satellites.clear();
    _observation.provenance.clear();
    _observation.monotonicTimestampUs = 0;
    quint64 nextExpiryUs = std::numeric_limits<quint64>::max();
    for (const auto& [constellation, state] : _constellations) {
        if (!state.viewReceiptUs && !state.useReceiptUs) {
            continue;
        }
        _observation.provenance.append(
            {constellation, state.viewReceiptUs, state.useReceiptUs, state.usedCount, state.usedIds});
        for (auto satellite : state.satellites) {
            const auto used = state.used.find({satellite.id, satellite.prn});
            satellite.used = std::nullopt;
            if (state.useReceiptUs) {
                if (state.usedIds) {
                    satellite.used = state.usedIds->contains(satellite.id);
                } else if (used != state.used.cend()) {
                    satellite.used = used->second;
                }
            }
            _observation.satellites.append(satellite);
        }
        for (const quint64 receipt : {state.viewReceiptUs, state.useReceiptUs}) {
            if (receipt) {
                _observation.monotonicTimestampUs = std::max(_observation.monotonicTimestampUs, receipt);
                nextExpiryUs = std::min(nextExpiryUs, receipt + static_cast<quint64>(_freshnessTimeoutMs) * 1000);
            }
        }
    }
    _scheduler->cancel(_expiryTask);
    _expiryTask = 0;
    if (nextExpiryUs != std::numeric_limits<quint64>::max()) {
        _expiryTask = _scheduler->schedule(this, std::chrono::microseconds(nextExpiryUs - nowUs), [this]() {
            _expiryTask = 0;
            _publish();
        });
    }
    _observation.revision = ++_revision;
    const GPSSatelliteObservation snapshot = _observation;
    emit observationChanged(snapshot);
}
