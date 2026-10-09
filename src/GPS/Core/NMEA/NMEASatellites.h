#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

#include "GPSReceiverReports.h"

namespace NMEA {
namespace SatelliteIds {
inline constexpr int FIRST_LOCAL_ID = 1;
inline constexpr int GLONASS_FIRST_NMEA_ID = 65;
inline constexpr int GLONASS_LAST_NMEA_ID = 96;
inline constexpr int GALILEO_FIRST_NMEA_ID = 301;
inline constexpr int GALILEO_LAST_NMEA_ID = 336;
inline constexpr int BEIDOU_EXTENDED_FIRST_NMEA_ID = 401;
inline constexpr int BEIDOU_EXTENDED_LAST_NMEA_ID = 463;
inline constexpr int BEIDOU_LEGACY_FIRST_NMEA_ID = 201;
inline constexpr int BEIDOU_LEGACY_LAST_NMEA_ID = 235;
inline constexpr int QZSS_FIRST_NMEA_ID = 193;
inline constexpr int QZSS_LAST_NMEA_ID = 202;
inline constexpr int SBAS_FIRST_NMEA_ID = 33;
inline constexpr int SBAS_LAST_NMEA_ID = 64;
inline constexpr int SBAS_FIRST_PRN = 120;
}  // namespace SatelliteIds

/// Canonical constellation-local identifier; preserve unknown/vendor ranges unchanged.
[[nodiscard]] inline int satelliteId(GPSConstellation constellation, int wireId)
{
    namespace Id = SatelliteIds;

    switch (constellation) {
        case GPSConstellation::GLONASS:
            return wireId >= Id::GLONASS_FIRST_NMEA_ID && wireId <= Id::GLONASS_LAST_NMEA_ID
                       ? wireId - Id::GLONASS_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::Galileo:
            return wireId >= Id::GALILEO_FIRST_NMEA_ID && wireId <= Id::GALILEO_LAST_NMEA_ID
                       ? wireId - Id::GALILEO_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::BeiDou:
            if (wireId >= Id::BEIDOU_EXTENDED_FIRST_NMEA_ID && wireId <= Id::BEIDOU_EXTENDED_LAST_NMEA_ID) {
                return wireId - Id::BEIDOU_EXTENDED_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID;
            }
            return wireId >= Id::BEIDOU_LEGACY_FIRST_NMEA_ID && wireId <= Id::BEIDOU_LEGACY_LAST_NMEA_ID
                       ? wireId - Id::BEIDOU_LEGACY_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::QZSS:
            return wireId >= Id::QZSS_FIRST_NMEA_ID && wireId <= Id::QZSS_LAST_NMEA_ID
                       ? wireId - Id::QZSS_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::SBAS:
            return wireId >= Id::SBAS_FIRST_NMEA_ID && wireId <= Id::SBAS_LAST_NMEA_ID
                       ? wireId - Id::SBAS_FIRST_NMEA_ID + Id::SBAS_FIRST_PRN
                       : wireId;
        default:
            return wireId;
    }
}

[[nodiscard]] GPSConstellation satelliteConstellation(std::string_view talker, std::optional<int> systemId,
                                                      std::optional<int> satelliteId);

struct Sentence;

struct SatelliteData
{
    uint16_t id = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
};

struct SatelliteSystem
{
    GPSConstellation constellation = GPSConstellation::Unknown;
    uint64_t inViewTimestampUs = 0;
    uint64_t inUseTimestampUs = 0;
    int inView = 0;
    std::optional<int> inUse;
};

using SatelliteEpoch = std::vector<SatelliteSystem>;

struct GSV
{
    /// Unknown denotes a combined GN report with identities resolved per satellite.
    GPSConstellation constellation = GPSConstellation::Unknown;
    int messages = 0;
    int message = 0;
    int satelliteCount = 0;
    int signal = -1;
    std::vector<SatelliteData> satellites = {};
};

[[nodiscard]] std::optional<GSV> gsv(const Sentence& input);

/// Bounded multipart/multisignal assembly; scheduling and delivery remain with the caller.
class SatelliteAssembler
{
public:
    static constexpr std::chrono::microseconds IDLE_TIMEOUT = std::chrono::milliseconds(150);
    static constexpr std::chrono::microseconds BATCH_TIMEOUT = std::chrono::seconds(1);

    struct Update
    {
        bool accepted = false;
        SatelliteEpoch completed;
    };

    void clear();

    Update ingest(const Sentence& input, uint64_t receivedAtUs);

    SatelliteEpoch flush();
    SatelliteEpoch flushDue(uint64_t nowUs);
    std::optional<uint64_t> deadlineUs() const;

private:
    struct View
    {
        int messages = 0;
        int count = 0;
        int next = 1;
        uint64_t timestamp = 0;
        std::vector<SatelliteData> satellites = {};

        bool complete() const { return messages > 0 && next == messages + 1; }
    };

    struct Used
    {
        uint64_t timestamp = 0;
        std::set<int> ids;
    };

    std::map<GPSConstellation, std::map<int, View>> _views;
    std::map<GPSConstellation, Used> _used;
    std::optional<int> _time;
    std::optional<uint64_t> _batchStartedUs;
    uint64_t _lastAcceptedUs = 0;
};
}  // namespace NMEA
