#include <algorithm>
#include <cmath>

#include "GPSFixQuality.h"
#include "GPSNativeData_p.h"

namespace {
GPSSatellite satelliteObservation(const GPSNativeSatelliteData& source, GPSConstellation constellation)
{
    GPSSatellite result;
    result.id = source.id;
    result.prn = source.prn;
    result.used = source.used;
    result.constellation = constellation;
    if (source.elevation && *source.elevation >= -90 && *source.elevation <= 90) {
        result.elevationDegrees = static_cast<float>(*source.elevation);
    }
    if (source.azimuth && *source.azimuth >= 0 && *source.azimuth <= 360) {
        result.normalizedAzimuthDegrees = static_cast<float>(*source.azimuth);
    }
    if (source.signal && *source.signal >= 0 && *source.signal <= 255) {
        result.signalStrength = static_cast<uint8_t>(*source.signal);
    }
    return result;
}
}  // namespace

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
    observation.monotonicTimestampUs = source.timestamp;
    observation.updateMode = source.constellation ? GPSSatelliteObservation::UpdateMode::ConstellationDelta
                                                  : GPSSatelliteObservation::UpdateMode::FullSnapshot;
    const auto viewCount =
        std::min({source.count, GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES, GPSSatelliteReport::MAX_SATELLITES});
    const auto systemFor = [&observation](GPSConstellation constellation) -> GPSSatelliteConstellation& {
        const auto found =
            std::find_if(observation.constellations.begin(), observation.constellations.end(),
                         [constellation](const auto& value) { return value.constellation == constellation; });
        if (found != observation.constellations.end()) {
            return *found;
        }
        auto& system = observation.constellations.emplaceBack();
        system.constellation = constellation;
        return system;
    };
    if (source.constellation || viewCount == 0) {
        systemFor(source.constellation.value_or(GPSConstellation::Unknown));
    }
    for (uint16_t i = 0; i < viewCount; ++i) {
        const auto& satellite = source.entries[i];
        const auto constellation = source.constellation.value_or(satellite.constellation);
        systemFor(constellation).view.satellites.append(satelliteObservation(satellite, constellation));
    }
    for (auto& system : observation.constellations) {
        system.view.receivedAtUs = source.timestamp;
        if (source.constellation && source.usage) {
            system.usage.receivedAtUs = source.usage->timestamp;
            const auto count = std::min(source.usage->count, GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES);
            system.usage.count = count;
            system.usage.ids.emplace(source.usage->ids.begin(), source.usage->ids.begin() + count);
        } else {
            int used = 0;
            bool known = !source.constellation || !system.view.satellites.isEmpty();
            for (const auto& satellite : system.view.satellites) {
                known &= satellite.used.has_value();
                used += satellite.used.value_or(false);
            }
            if (known) {
                system.usage.receivedAtUs = source.timestamp;
                system.usage.count = used;
            }
        }
    }
    _latestReceiptUs =
        nowUs ? nowUs : std::max({_latestReceiptUs, source.timestamp, source.usage ? source.usage->timestamp : 0});
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
    for (const auto& system : accepted.constellations) {
        snapshot.timestampUs = std::max<uint64_t>(snapshot.timestampUs, system.view.receivedAtUs);
        for (const auto& satellite : system.view.satellites) {
            if (snapshot.count == GPSSatelliteReport::MAX_SATELLITES) {
                break;
            }
            auto& entry = snapshot.satellites[snapshot.count++];
            entry.id = satellite.id;
            entry.prn = satellite.prn;
            entry.constellation = system.constellation;
            entry.used = satellite.used;
            entry.elevationDegrees = satellite.elevationDegrees;
            entry.azimuthDegrees = satellite.normalizedAzimuthDegrees;
            entry.signalStrength = satellite.signalStrength;
            entry.inViewTimestampUs = system.view.receivedAtUs;
            entry.inUseTimestampUs = system.usage.receivedAtUs;
        }
    }
    return snapshot;
}
}  // namespace GPSNativeData
