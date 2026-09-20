#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

#include "GPSSatelliteObservation.h"

/// Timestamp acceptance and constellation retention, independent of notification scheduling.
class GPSSatelliteState
{
public:
    explicit GPSSatelliteState(int freshnessTimeoutMs = 5000)
        : _freshnessTimeoutMs(std::max(1, freshnessTimeoutMs))
    {}

    void reset()
    {
        _constellations.clear();
        _clearedThroughUs = 0;
        _fullSnapshotReceiptUs = 0;
    }

    void clear(quint64 nowUs)
    {
        _constellations.clear();
        _clearedThroughUs = nowUs;
    }

    void setFreshnessTimeoutMs(int value) { _freshnessTimeoutMs = std::max(1, value); }

    int freshnessTimeoutMs() const { return _freshnessTimeoutMs; }

    void updateObservation(const GPSSatelliteObservation& observation, quint64 nowUs)
    {
        _expire(nowUs);
        std::map<GPSConstellation, QList<GPSSatellite>> satellitesByConstellation;
        for (const auto& satellite : observation.satellites) {
            satellitesByConstellation[satellite.constellation].append(satellite);
        }
        auto reports = observation.provenance;
        if (reports.isEmpty() && observation.satellites.isEmpty() &&
            observation.updateMode == GPSSatelliteObservation::UpdateMode::ConstellationDelta) {
            return;
        }
        if (reports.isEmpty()) {
            if (satellitesByConstellation.empty()) {
                satellitesByConstellation[GPSConstellation::Unknown] = {};
            }
            for (const auto& [constellation, satellites] : satellitesByConstellation) {
                int count = 0;
                bool known = true;
                for (const auto& satellite : satellites) {
                    count += satellite.used.value_or(false) ? 1 : 0;
                    known &= satellite.used.has_value();
                }
                // Populate in place; GCC 13 -O3 misdiagnoses the aggregate append's disengaged optional<QList>.
                auto& report = reports.emplaceBack();
                report.constellation = constellation;
                report.inViewTimestampUs = observation.monotonicTimestampUs;
                if (known) {
                    report.inUseTimestampUs = observation.monotonicTimestampUs;
                    report.satellitesUsed = count;
                }
            }
        }
        const bool fullSnapshot = observation.updateMode == GPSSatelliteObservation::UpdateMode::FullSnapshot;
        quint64 fullReceipt = observation.monotonicTimestampUs;
        for (const auto& report : reports) {
            fullReceipt = std::max({fullReceipt, report.inViewTimestampUs, report.inUseTimestampUs});
        }
        if (fullSnapshot) {
            if (!fullReceipt || fullReceipt > nowUs || fullReceipt <= _clearedThroughUs ||
                fullReceipt < _fullSnapshotReceiptUs || !_remaining(fullReceipt, nowUs)) {
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
        const QList<GPSSatellite> emptySatellites;
        for (const auto& report : reports) {
            if (report.constellation < GPSConstellation::Unknown || report.constellation > GPSConstellation::NavIC) {
                continue;
            }
            auto& state = _constellations[report.constellation];
            const auto bucket = satellitesByConstellation.find(report.constellation);
            const auto& satellites = bucket != satellitesByConstellation.end() ? bucket->second : emptySatellites;
            if ((fullSnapshot || report.inViewTimestampUs >= _fullSnapshotReceiptUs) &&
                _accept(report.inViewTimestampUs, state.viewReceiptUs, state.viewRetiredThroughUs, nowUs)) {
                state.viewReceiptUs = report.inViewTimestampUs;
                state.satellites = satellites;
            }
            if ((fullSnapshot || report.inUseTimestampUs >= _fullSnapshotReceiptUs) &&
                (!report.satellitesUsed || *report.satellitesUsed >= 0) &&
                _accept(report.inUseTimestampUs, state.useReceiptUs, state.useRetiredThroughUs, nowUs)) {
                state.useReceiptUs = report.inUseTimestampUs;
                state.usedCount = report.satellitesUsed;
                state.usedIds = report.satellitesUsed ? report.usedSatelliteIds : std::nullopt;
                state.used.clear();
                for (const auto& satellite : satellites) {
                    if (report.satellitesUsed && satellite.used) {
                        state.used[{satellite.id, satellite.prn}] = *satellite.used;
                    }
                }
            }
        }
    }

    GPSSatelliteObservation snapshot(quint64 nowUs)
    {
        _expire(nowUs);
        GPSSatelliteObservation observation;
        for (const auto& [constellation, state] : _constellations) {
            if (!state.viewReceiptUs && !state.useReceiptUs) {
                continue;
            }
            observation.provenance.append(
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
                observation.satellites.append(satellite);
            }
            observation.monotonicTimestampUs =
                std::max({observation.monotonicTimestampUs, state.viewReceiptUs, state.useReceiptUs});
        }
        return observation;
    }

    std::optional<quint64> nextExpiryUs() const
    {
        std::optional<quint64> deadline;
        for (const auto& [constellation, state] : _constellations) {
            for (const auto receipt : {state.viewReceiptUs, state.useReceiptUs}) {
                if (receipt) {
                    const auto expiry = receipt + quint64(_freshnessTimeoutMs) * 1000;
                    deadline = deadline ? std::min(*deadline, expiry) : expiry;
                }
            }
        }
        return deadline;
    }

private:
    struct ConstellationState
    {
        quint64 viewReceiptUs = 0;
        quint64 useReceiptUs = 0;
        quint64 viewRetiredThroughUs = 0;
        quint64 useRetiredThroughUs = 0;
        QList<GPSSatellite> satellites;
        std::map<std::pair<int, int>, bool> used;
        std::optional<int> usedCount;
        std::optional<QList<int>> usedIds;
    };

    quint64 _remaining(quint64 receipt, quint64 nowUs) const
    {
        const auto lifetime = quint64(_freshnessTimeoutMs) * 1000;
        return receipt && receipt <= nowUs && nowUs - receipt < lifetime ? lifetime - (nowUs - receipt) : 0;
    }

    bool _accept(quint64 receipt, quint64 current, quint64& retired, quint64 nowUs) const
    {
        if (!receipt || receipt <= _clearedThroughUs || receipt <= retired || receipt < current || receipt > nowUs) {
            return false;
        }
        if (!_remaining(receipt, nowUs)) {
            retired = std::max(retired, receipt);
            return false;
        }
        return true;
    }

    void _expire(quint64 nowUs)
    {
        for (auto& [constellation, state] : _constellations) {
            if (state.viewReceiptUs && !_remaining(state.viewReceiptUs, nowUs)) {
                state.viewRetiredThroughUs = std::max(state.viewRetiredThroughUs, state.viewReceiptUs);
                state.viewReceiptUs = 0;
                state.satellites.clear();
            }
            if (state.useReceiptUs && !_remaining(state.useReceiptUs, nowUs)) {
                state.useRetiredThroughUs = std::max(state.useRetiredThroughUs, state.useReceiptUs);
                state.useReceiptUs = 0;
                state.usedCount.reset();
                state.used.clear();
                state.usedIds.reset();
            }
        }
    }

    std::map<GPSConstellation, ConstellationState> _constellations;
    int _freshnessTimeoutMs;
    quint64 _clearedThroughUs = 0;
    quint64 _fullSnapshotReceiptUs = 0;
};
