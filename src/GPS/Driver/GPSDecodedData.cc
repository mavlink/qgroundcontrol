#include <algorithm>
#include <cmath>

#include "GPSDecodedData_p.h"
#include "GPSFixQuality.h"

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
    if (navigation.satellitesUsed == std::numeric_limits<uint8_t>::max()) {
        navigation.satellitesUsed.reset();
    }
    auto& integrity = result.integrity;
    integrity.jamming.state = GPSIntegrityReport::jammingStateFromValue(static_cast<int>(diagnostic.jamming.state));
    integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue(static_cast<int>(diagnostic.spoofing.state));
    integrity.corrections.use =
        GPSIntegrityReport::correctionUseFromValue(static_cast<int>(diagnostic.corrections.use));
    // Navigation epochs retain their first receipt; diagnostics may arrive before that epoch is published.
    integrity = integrity.freshAt(nowUs ? nowUs : std::max(navigation.timestampUs, diagnostic.timestampUs));
    return result;
}

GPSSatelliteReport SatelliteSnapshot::update(const GPSDecodedSatellites& source, uint64_t nowUs)
{
    GPSSatelliteObservation observation;
    observation.updateMode = source.fullSnapshot ? GPSSatelliteObservation::UpdateMode::FullSnapshot
                                                 : GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    const auto constellationCount = std::min<size_t>(source.count, source.constellations.size());
    for (size_t index = 0; index < constellationCount; ++index) {
        const auto& native = source.constellations[index];
        auto& system = observation.constellations.emplaceBack();
        system.constellation = native.constellation;
        system.view = {native.inViewTimestampUs,
                       std::clamp(native.inView, 0, int(GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES))};
        system.usage = {native.inUseTimestampUs,
                        native.inUse ? std::optional<int>(std::clamp(
                                           *native.inUse, 0, int(GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES)))
                                     : std::nullopt};
        observation.monotonicTimestampUs = std::max({static_cast<uint64_t>(observation.monotonicTimestampUs),
                                                     native.inViewTimestampUs, native.inUseTimestampUs});
    }
    _latestReceiptUs = nowUs ? nowUs : std::max<uint64_t>(_latestReceiptUs, observation.monotonicTimestampUs);
    _state.updateObservation(observation, _latestReceiptUs);
    return _snapshot(_latestReceiptUs, UsageSelection::ViewThenCountOnly);
}

GPSSatelliteReport SatelliteSnapshot::update(const GPSDecodedSatelliteUsage& source, uint64_t nowUs)
{
    const uint64_t receipt = nowUs ? nowUs : source.timestampUs;
    _countOnlyUsage = source.usedCount;
    _countOnlyUsageReceiptUs = receipt;
    _latestReceiptUs = std::max(_latestReceiptUs, receipt);
    _countOnlyUsageIsLatest = true;
    return _snapshot(_latestReceiptUs, UsageSelection::CountOnly);
}

std::optional<GPSSatelliteReport> SatelliteSnapshot::expire(uint64_t nowUs)
{
    const auto deadline = _state.nextExpiryUs();
    if (!deadline || nowUs < *deadline) {
        return std::nullopt;
    }
    _latestReceiptUs = nowUs;
    return _snapshot(nowUs, UsageSelection::ViewThenCountOnly);
}

SatelliteSnapshot::SatelliteProjection SatelliteSnapshot::_project(uint64_t nowUs)
{
    const auto accepted = _state.snapshot(nowUs);
    SatelliteProjection projection;
    bool usedKnown = true;
    int used = 0;
    int inView = 0;
    for (const auto& system : accepted.constellations) {
        if (!system.view.receivedAtUs) {
            continue;
        }
        projection.report.timestampUs = std::max<uint64_t>(projection.report.timestampUs, system.view.receivedAtUs);
        inView += system.view.count;
        if (system.view.count <= 0) {
            projection.viewUsageReceiptUs = std::max<uint64_t>(projection.viewUsageReceiptUs, system.view.receivedAtUs);
            continue;
        }
        if (system.usage.receivedAtUs && system.usage.count) {
            used += *system.usage.count;
            projection.viewUsageReceiptUs =
                std::max<uint64_t>(projection.viewUsageReceiptUs, system.usage.receivedAtUs);
        } else {
            usedKnown = false;
        }
    }
    if (projection.report.timestampUs) {
        projection.report.inView = inView;
        if (inView == 0) {
            projection.viewUsed = 0;
            projection.viewUsageReceiptUs = projection.report.timestampUs;
        } else if (usedKnown) {
            projection.viewUsed = used;
        }
    }
    return projection;
}

GPSSatelliteReport SatelliteSnapshot::_snapshot(uint64_t nowUs, UsageSelection usageSelection)
{
    auto projection = _project(nowUs);
    _countOnlyUsageIsLatest = _countOnlyUsageReceiptUs && _countOnlyUsageReceiptUs >= projection.viewUsageReceiptUs;
    if (usageSelection == UsageSelection::CountOnly || (_countOnlyUsageIsLatest && !projection.viewUsed)) {
        projection.report.used = _countOnlyUsage;
    } else if (projection.viewUsed) {
        projection.report.used = projection.viewUsed;
    } else {
        projection.report.used = _countOnlyUsage;
    }
    return projection.report;
}
}  // namespace GPSDecodedData
