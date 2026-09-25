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
    static constexpr std::chrono::milliseconds FRESHNESS_TIMEOUT{5000};

    explicit GPSSatelliteState(std::chrono::milliseconds freshnessTimeout = FRESHNESS_TIMEOUT)
        : _freshnessTimeout(std::max(std::chrono::milliseconds{1}, freshnessTimeout))
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

    void setFreshnessTimeout(std::chrono::milliseconds value)
    {
        _freshnessTimeout = std::max(std::chrono::milliseconds{1}, value);
    }

    std::chrono::milliseconds freshnessTimeout() const { return _freshnessTimeout; }

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
                fullReceipt < _fullSnapshotReceiptUs || _expired(fullReceipt, nowUs)) {
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
                    const auto expiry =
                        receipt + static_cast<quint64>(std::chrono::microseconds(_freshnessTimeout).count());
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

    bool _expired(quint64 receipt, quint64 nowUs) const
    {
        return MonotonicClock::remaining(receipt, nowUs, _freshnessTimeout) == std::chrono::microseconds::zero();
    }

    bool _accept(quint64 receipt, quint64 current, quint64& retired, quint64 nowUs) const
    {
        if (!receipt || receipt <= _clearedThroughUs || receipt <= retired || receipt < current || receipt > nowUs) {
            return false;
        }
        if (_expired(receipt, nowUs)) {
            retired = std::max(retired, receipt);
            return false;
        }
        return true;
    }

    void _expire(quint64 nowUs)
    {
        for (auto& [constellation, state] : _constellations) {
            if (state.view.receivedAtUs && _expired(state.view.receivedAtUs, nowUs)) {
                state.view.retire(state.view.receivedAtUs);
            }
            if (state.usage.receivedAtUs && _expired(state.usage.receivedAtUs, nowUs)) {
                state.usage.retire(state.usage.receivedAtUs);
            }
        }
    }

    std::map<GPSConstellation, ConstellationState> _constellations;
    std::chrono::milliseconds _freshnessTimeout;
    quint64 _clearedThroughUs = 0;
    quint64 _fullSnapshotReceiptUs = 0;
};
