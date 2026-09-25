#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>

#include "GPSSatelliteObservation.h"
#include "MonotonicClock.h"

/// Timestamp acceptance and constellation retention, independent of notification scheduling.
class GPSSatelliteState
{
public:
    /// Receipt age at which a constellation's view or usage count is retired.
    static constexpr int FRESHNESS_TIMEOUT_MS = 5000;

    explicit GPSSatelliteState(int freshnessTimeoutMs = FRESHNESS_TIMEOUT_MS)
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
        const auto& reports = observation.constellations;
        const bool fullSnapshot = observation.updateMode == GPSSatelliteObservation::UpdateMode::FullSnapshot;
        quint64 fullReceipt = observation.monotonicTimestampUs;
        for (const auto& report : reports) {
            fullReceipt = std::max({fullReceipt, report.view.receivedAtUs, report.usage.receivedAtUs});
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
                if ((report == reports.cend() || !report->view.receivedAtUs) &&
                    state.view.receivedAtUs <= fullReceipt) {
                    state.view.retire(fullReceipt);
                }
                if ((report == reports.cend() || !report->usage.receivedAtUs) &&
                    state.usage.receivedAtUs <= fullReceipt) {
                    state.usage.retire(fullReceipt);
                }
            }
        }
        for (const auto& report : reports) {
            if (report.constellation < GPSConstellation::Unknown || report.constellation > GPSConstellation::NavIC) {
                continue;
            }
            auto& state = _constellations[report.constellation];
            if ((fullSnapshot || report.view.receivedAtUs >= _fullSnapshotReceiptUs) && report.view.count >= 0 &&
                _accept(report.view.receivedAtUs, state.view.receivedAtUs, state.view.retiredThroughUs, nowUs)) {
                state.view.receivedAtUs = report.view.receivedAtUs;
                state.view.count = report.view.count;
            }
            if ((fullSnapshot || report.usage.receivedAtUs >= _fullSnapshotReceiptUs) &&
                (!report.usage.count || *report.usage.count >= 0) &&
                _accept(report.usage.receivedAtUs, state.usage.receivedAtUs, state.usage.retiredThroughUs, nowUs)) {
                state.usage.receivedAtUs = report.usage.receivedAtUs;
                state.usage.count = report.usage.count;
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
            auto& system = observation.constellations.emplaceBack();
            system.constellation = constellation;
            system.view.receivedAtUs = state.view.receivedAtUs;
            system.view.count = state.view.count;
            system.usage = {state.usage.receivedAtUs, state.usage.count};
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
        int count = 0;

        void retire(quint64 throughUs) { *this = ViewState{.retiredThroughUs = std::max(retiredThroughUs, throughUs)}; }
    };

    struct UsageState
    {
        quint64 receivedAtUs = 0;
        quint64 retiredThroughUs = 0;
        std::optional<int> count = std::nullopt;

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
