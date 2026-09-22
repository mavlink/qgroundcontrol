#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

#include "GPSSatelliteObservation.h"
#include "MonotonicClock.h"

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
                if ((report == reports.cend() || !report->inViewTimestampUs) &&
                    state.view.receivedAtUs <= fullReceipt) {
                    state.view.retire(fullReceipt);
                }
                if ((report == reports.cend() || !report->inUseTimestampUs) &&
                    state.usage.receivedAtUs <= fullReceipt) {
                    state.usage.retire(fullReceipt);
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
                _accept(report.inViewTimestampUs, state.view.receivedAtUs, state.view.retiredThroughUs, nowUs)) {
                state.view.receivedAtUs = report.inViewTimestampUs;
                state.view.satellites = satellites;
            }
            if ((fullSnapshot || report.inUseTimestampUs >= _fullSnapshotReceiptUs) &&
                (!report.satellitesUsed || *report.satellitesUsed >= 0) &&
                _accept(report.inUseTimestampUs, state.usage.receivedAtUs, state.usage.retiredThroughUs, nowUs)) {
                state.usage.receivedAtUs = report.inUseTimestampUs;
                state.usage.count = report.satellitesUsed;
                state.usage.ids = report.satellitesUsed ? report.usedSatelliteIds : std::nullopt;
                state.usage.flags.clear();
                for (const auto& satellite : satellites) {
                    if (report.satellitesUsed && satellite.used) {
                        state.usage.flags[{satellite.id, satellite.prn}] = *satellite.used;
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
            if (!state.view.receivedAtUs && !state.usage.receivedAtUs) {
                continue;
            }
            observation.provenance.append(
                {constellation, state.view.receivedAtUs, state.usage.receivedAtUs, state.usage.count, state.usage.ids});
            for (auto satellite : state.view.satellites) {
                const auto used = state.usage.flags.find({satellite.id, satellite.prn});
                satellite.used = std::nullopt;
                if (state.usage.receivedAtUs) {
                    if (state.usage.ids) {
                        satellite.used = state.usage.ids->contains(satellite.id);
                    } else if (used != state.usage.flags.cend()) {
                        satellite.used = used->second;
                    }
                }
                observation.satellites.append(satellite);
            }
            observation.monotonicTimestampUs =
                std::max({observation.monotonicTimestampUs, state.view.receivedAtUs, state.usage.receivedAtUs});
        }
        return observation;
    }

    std::optional<quint64> nextExpiryUs() const
    {
        std::optional<quint64> deadline;
        for (const auto& [constellation, state] : _constellations) {
            for (const auto receipt : {state.view.receivedAtUs, state.usage.receivedAtUs}) {
                if (receipt) {
                    const auto expiry = receipt + quint64(_freshnessTimeoutMs) * 1000;
                    deadline = deadline ? std::min(*deadline, expiry) : expiry;
                }
            }
        }
        return deadline;
    }

private:
    struct ViewState
    {
        quint64 receivedAtUs = 0;
        quint64 retiredThroughUs = 0;
        QList<GPSSatellite> satellites = {};

        void retire(quint64 throughUs) { *this = ViewState{.retiredThroughUs = std::max(retiredThroughUs, throughUs)}; }
    };

    struct UsageState
    {
        quint64 receivedAtUs = 0;
        quint64 retiredThroughUs = 0;
        std::map<std::pair<int, int>, bool> flags = {};
        std::optional<int> count = std::nullopt;
        std::optional<QList<int>> ids = std::nullopt;

        void retire(quint64 throughUs)
        {
            *this = UsageState{.retiredThroughUs = std::max(retiredThroughUs, throughUs)};
        }
    };

    struct ConstellationState
    {
        ViewState view;
        UsageState usage;
    };

    quint64 _remaining(quint64 receipt, quint64 nowUs) const
    {
        return static_cast<quint64>(
            MonotonicClock::remaining(receipt, nowUs, std::chrono::milliseconds(_freshnessTimeoutMs)).count());
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
            if (state.view.receivedAtUs && !_remaining(state.view.receivedAtUs, nowUs)) {
                state.view.retire(state.view.receivedAtUs);
            }
            if (state.usage.receivedAtUs && !_remaining(state.usage.receivedAtUs, nowUs)) {
                state.usage.retire(state.usage.receivedAtUs);
            }
        }
    }

    std::map<GPSConstellation, ConstellationState> _constellations;
    int _freshnessTimeoutMs;
    quint64 _clearedThroughUs = 0;
    quint64 _fullSnapshotReceiptUs = 0;
};
