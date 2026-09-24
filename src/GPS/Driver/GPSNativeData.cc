#include <algorithm>
#include <cmath>

#include "GPSFixQuality.h"
#include "GPSNativeData_p.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSIntegrityReport& diagnostic, uint64_t nowUs)
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

GPSSatelliteReport SatelliteSnapshot::update(const GPSNativeSatelliteReport& source, uint64_t nowUs)
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
                       std::clamp(native.inView, 0, int(GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES))};
        system.usage = {native.inUseTimestampUs,
                        native.inUse ? std::optional<int>(std::clamp(
                                           *native.inUse, 0, int(GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES)))
                                     : std::nullopt};
        observation.monotonicTimestampUs = std::max({static_cast<uint64_t>(observation.monotonicTimestampUs),
                                                     native.inViewTimestampUs, native.inUseTimestampUs});
    }
    _latestReceiptUs = nowUs ? nowUs : std::max<uint64_t>(_latestReceiptUs, observation.monotonicTimestampUs);
    _state.updateObservation(observation, _latestReceiptUs);
    return _snapshot(_latestReceiptUs);
}

std::optional<GPSSatelliteReport> SatelliteSnapshot::expire(uint64_t nowUs)
{
    const auto deadline = _state.nextExpiryUs();
    if (!deadline || nowUs < *deadline) {
        return std::nullopt;
    }
    _latestReceiptUs = nowUs;
    return _snapshot(nowUs);
}

GPSSatelliteReport SatelliteSnapshot::_snapshot(uint64_t nowUs)
{
    const auto accepted = _state.snapshot(nowUs);
    GPSSatelliteReport snapshot;
    bool usedKnown = true;
    int used = 0;
    for (const auto& system : accepted.constellations) {
        snapshot.timestampUs = std::max<uint64_t>(snapshot.timestampUs, system.view.receivedAtUs);
        if (!system.view.receivedAtUs) {
            continue;
        }
        snapshot.inView += system.view.count;
        if (system.view.count <= 0) {
            continue;
        }
        if (system.usage.receivedAtUs && system.usage.count) {
            used += *system.usage.count;
        } else {
            usedKnown = false;
        }
    }
    if (snapshot.timestampUs && snapshot.inView == 0) {
        snapshot.used = 0;
    } else if (snapshot.timestampUs && usedKnown) {
        snapshot.used = used;
    }
    return snapshot;
}
}  // namespace GPSNativeData
