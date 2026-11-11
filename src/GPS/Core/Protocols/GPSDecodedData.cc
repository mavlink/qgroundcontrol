#include "GPSDecodedData.h"

#include <algorithm>
#include <cmath>
#include <span>

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
    integrity = integrity.freshAt(nowUs ? nowUs : (std::max) (navigation.timestampUs, diagnostic.timestampUs));
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
    return _report();
}

GPSSatelliteReport SatelliteCounts::update(const GPSDecodedSatelliteUsage& source, uint64_t nowUs)
{
    _accept(_countOnlyUsed, source.timestampUs, source.usedCount, nowUs);
    (void) _expire(nowUs);
    return _report();
}

std::optional<GPSSatelliteReport> SatelliteCounts::expire(uint64_t nowUs)
{
    if (!_expire(nowUs)) {
        return std::nullopt;
    }
    return _report();
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
    const auto retire = [&expired, nowUs](Count& count) {
        if (count.receivedAtUs &&
            !MonotonicClock::fresh(count.receivedAtUs, nowUs, SatelliteCounts::FRESHNESS_TIMEOUT)) {
            count = {};
            expired = true;
        }
    };
    for (auto& [constellation, counts] : _constellations) {
        retire(counts.inView);
        retire(counts.used);
    }
    retire(_countOnlyUsed);
    return expired;
}

GPSSatelliteReport SatelliteCounts::_report() const
{
    GPSSatelliteReport report;
    int inView = 0;
    int used = 0;
    bool usedKnown = true;
    for (const auto& [constellation, counts] : _constellations) {
        if (!counts.inView.receivedAtUs) {
            continue;
        }
        report.timestampUs = (std::max) (report.timestampUs, counts.inView.receivedAtUs);
        const int visible = counts.inView.value.value_or(0);
        inView += visible;
        if (visible == 0) {
            continue;
        }
        const std::optional<int> constellationUsed = counts.used.receivedAtUs ? counts.used.value : std::nullopt;
        if (constellationUsed) {
            used += *constellationUsed;
        } else {
            usedKnown = false;
        }
        report.addConstellation({.constellation = constellation, .inView = visible, .used = constellationUsed});
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
    report.used = viewUsed ? viewUsed : _countOnlyUsed.value;
    return report;
}
}  // namespace GPSDecodedData
