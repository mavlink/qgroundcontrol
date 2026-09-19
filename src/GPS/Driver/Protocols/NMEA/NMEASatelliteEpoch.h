#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "GPSConstellation.h"

namespace NMEA {
struct Sentence;

struct SatelliteData
{
    uint16_t id = 0;
    uint16_t prn = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
    std::optional<double> elevation;
    std::optional<double> azimuth;
    std::optional<int> signal;
};

struct SatelliteSystem
{
    GPSConstellation constellation = GPSConstellation::Unknown;
    uint64_t inViewTimestampUs = 0;
    uint64_t inUseTimestampUs = 0;
    std::vector<SatelliteData> satellites = {};
    std::optional<std::set<int>> usedIds;
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

std::optional<GSV> gsv(const Sentence& input);

/// Bounded multipart/multisignal assembly; scheduling and delivery remain with the caller.
class SatelliteAssembler
{
public:
    struct Update
    {
        bool accepted = false;
        SatelliteEpoch completed;
    };

    void clear();

    Update ingest(const Sentence& input, uint64_t receivedAtUs);

    SatelliteEpoch flush();

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
};
}  // namespace NMEA
