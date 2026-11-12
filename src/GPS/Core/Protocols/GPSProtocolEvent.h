#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QFlags>

#include "GPSReceiverReports.h"
#include "GPSType.h"

/// What a decode or receive cycle produced. Position and satellite updates are navigation data; activity is protocol
/// traffic that proves the receiver is talking, such as acknowledgements, diagnostics and corrections.
enum class GPSReceiveUpdate : uint8_t
{
    Position = 0x1,
    Satellites = 0x2,
    Activity = 0x4,
};
Q_DECLARE_FLAGS(GPSReceiveUpdates, GPSReceiveUpdate)
Q_DECLARE_OPERATORS_FOR_FLAGS(GPSReceiveUpdates)

/// Decoded navigation plus whether the producer accepted its velocity solution.
struct GPSDecodedPosition
{
    GPSNavigationValues navigation{};
    bool velocityValid{};
};

struct GPSDecodedSatellites
{
    static constexpr uint16_t SAT_INFO_MAX_SATELLITES = GPSSatelliteReport::MAX_SATELLITES;
    static constexpr uint8_t MAX_CONSTELLATIONS = 8;

    struct Constellation
    {
        GPSConstellation constellation = GPSConstellation::Unknown;
        uint64_t inViewTimestampUs = 0;
        int inView = 0;
        uint64_t inUseTimestampUs = 0;
        std::optional<int> inUse = std::nullopt;
    };

    Constellation* ensureConstellation(GPSConstellation constellation)
    {
        for (uint8_t index = 0; index < count; ++index) {
            if (constellations[index].constellation == constellation) {
                return &constellations[index];
            }
        }
        if (count >= constellations.size()) {
            return nullptr;
        }
        auto& report = constellations[count++];
        report.constellation = constellation;
        return &report;
    }

    /// Lists @a satellite as in view of a constellation the caller counts; dropped once SAT_INFO_MAX_SATELLITES are
    /// listed.
    void listSatellite(const GPSSatellite& satellite)
    {
        if (satelliteCount < satellites.size()) {
            satellites[satelliteCount++] = satellite;
        }
    }

    /// Counts @a satellite in view of its constellation, and in use or not where it reports that, and lists it.
    /// Dropped when MAX_CONSTELLATIONS other constellations are already present.
    void addSatellite(const GPSSatellite& satellite)
    {
        auto* system = ensureConstellation(satellite.constellation);
        if (!system) {
            return;
        }
        ++system->inView;
        if (satellite.use != GPSSatellite::Use::Unknown) {
            system->inUse = system->inUse.value_or(0) + (satellite.use == GPSSatellite::Use::Used ? 1 : 0);
        }
        listSatellite(satellite);
    }

    /// The satellites listed, in the order the decoder listed them.
    [[nodiscard]] std::span<const GPSSatellite> satellitesInView() const
    {
        return std::span(satellites).first(std::min<size_t>(satelliteCount, satellites.size()));
    }

    // Full snapshots retire omitted constellations; deltas preserve them.
    bool fullSnapshot = true;
    uint8_t count = 0;
    std::array<Constellation, MAX_CONSTELLATIONS> constellations{};
    /// The satellites in view of the constellations above, where the receiver describes them: the first
    /// satelliteCount entries.
    uint8_t satelliteCount = 0;
    std::array<GPSSatellite, SAT_INFO_MAX_SATELLITES> satellites{};
};

struct GPSDecodedSatelliteUsage
{
    uint64_t timestampUs = 0;
    std::optional<int> usedCount = std::nullopt;
};

/// A reported used-satellite count; receivers report UINT8_MAX and above when the count is unknown.
[[nodiscard]] inline std::optional<uint8_t> gpsSatellitesUsed(std::optional<unsigned> count)
{
    return count && *count < UINT8_MAX ? std::optional<uint8_t>(static_cast<uint8_t>(*count)) : std::nullopt;
}

/// One RTCM3 frame the receiver produced, including its header and CRC.
struct GPSRTCMFrame
{
    QByteArray bytes{};
};

/// The protocol a passive input decodes, published when it is identified and when it changes: the family whose native
/// protocol the stream carries, or GPSType::Passive for standard NMEA.
struct GPSInputProtocol
{
    GPSType family = GPSType::Passive;
};

using GPSProtocolEvent = std::variant<GPSDecodedPosition, GPSIntegrityReport, GPSDecodedSatellites,
                                      GPSDecodedSatelliteUsage, GPSBaseStatus, GPSRTCMFrame, GPSInputProtocol>;

/// Events decoded from one span of received bytes, in publication order.
struct GPSEventBatch
{
    std::vector<GPSProtocolEvent> events{};
    GPSReceiveUpdates updates{};
};
