#include "GPSDecodedData.h"

#include <algorithm>
#include <cmath>

#include "MonotonicClock.h"

namespace GPSDecodedData {

GPSPositionReport position(const GPSDecodedPosition& source, const GPSIntegrityReport& diagnostic, uint64_t nowUs)
{
    GPSPositionReport result{.navigation = source.navigation, .integrity = diagnostic};
    auto& navigation = result.navigation;
    navigation.fixType = gpsFixQualityFromValue(static_cast<int>(navigation.fixType));
    if (!source.velocityValid) {
        navigation.speedMetersPerSecond = NAN;
        navigation.courseRadians = NAN;
    }
    auto& integrity = result.integrity;
    integrity.jamming.state = GPSIntegrityReport::jammingStateFromValue(static_cast<int>(diagnostic.jamming.state));
    integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue(static_cast<int>(diagnostic.spoofing.state));
    // Navigation epochs retain their first receipt; diagnostics may arrive before that epoch is published.
    integrity = integrity.freshAt(nowUs ? nowUs : std::max(navigation.timestampUs, diagnostic.timestampUs));
    return result;
}

GPSSatelliteReport SatelliteCounts::update(const GPSDecodedSatellites& source, uint64_t nowUs)
{
    if (source.fullSnapshot) {
        for (auto& [constellation, counts] : _constellations) {
            counts = {};
        }
    }
    constexpr int LIMIT = GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES;
    const auto reported =
        std::span(source.constellations).first(std::min<size_t>(source.count, source.constellations.size()));
    for (const auto& native : reported) {
        auto& counts = _constellations[native.constellation];
        _accept(counts.inView, native.inViewTimestampUs, std::clamp(native.inView, 0, LIMIT), nowUs);
        _accept(counts.used, native.inUseTimestampUs,
                native.inUse ? std::optional<int>(std::clamp(*native.inUse, 0, LIMIT)) : std::nullopt, nowUs);
    }
    (void) _expire(nowUs);
    return _report(false);
}

GPSSatelliteReport SatelliteCounts::update(const GPSDecodedSatelliteUsage& source, uint64_t nowUs)
{
    _countOnlyUsed = source.usedCount;
    (void) _expire(nowUs);
    return _report(true);
}

std::optional<GPSSatelliteReport> SatelliteCounts::expire(uint64_t nowUs)
{
    if (!_expire(nowUs)) {
        return std::nullopt;
    }
    return _report(false);
}

void SatelliteCounts::_accept(Count& count, uint64_t receivedAtUs, std::optional<int> value, uint64_t nowUs)
{
    if (receivedAtUs >= count.receivedAtUs &&
        MonotonicClock::fresh(receivedAtUs, nowUs, SatelliteCounts::FRESHNESS_TIMEOUT)) {
        count = {receivedAtUs, value};
    }
}

bool SatelliteCounts::_expire(uint64_t nowUs)
{
    bool expired = false;
    for (auto& [constellation, counts] : _constellations) {
        for (Count* count : {&counts.inView, &counts.used}) {
            if (count->receivedAtUs &&
                !MonotonicClock::fresh(count->receivedAtUs, nowUs, SatelliteCounts::FRESHNESS_TIMEOUT)) {
                *count = {};
                expired = true;
            }
        }
    }
    return expired;
}

GPSSatelliteReport SatelliteCounts::_report(bool countOnlyUsed) const
{
    GPSSatelliteReport report;
    int inView = 0;
    int used = 0;
    bool usedKnown = true;
    for (const auto& [constellation, counts] : _constellations) {
        if (!counts.inView.receivedAtUs) {
            continue;
        }
        report.timestampUs = std::max(report.timestampUs, counts.inView.receivedAtUs);
        const int visible = counts.inView.value.value_or(0);
        inView += visible;
        if (visible == 0) {
            continue;
        }
        if (counts.used.receivedAtUs && counts.used.value) {
            used += *counts.used.value;
        } else {
            usedKnown = false;
        }
    }
    std::optional<int> viewUsed;
    if (report.timestampUs) {
        report.inView = inView;
        if (inView == 0) {
            viewUsed = 0;
        } else if (usedKnown) {
            viewUsed = used;
        }
    }
    report.used = countOnlyUsed || !viewUsed ? _countOnlyUsed : viewUsed;
    return report;
}
}  // namespace GPSDecodedData
