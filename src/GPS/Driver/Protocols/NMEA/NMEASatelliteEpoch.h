#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "GPSSatelliteData.h"
#include "NMEAConstellation.h"
#include "NMEASentence.h"

namespace NMEA {
struct SatelliteSystem
{
    GPSConstellation constellation = GPSConstellation::Unknown;
    uint64_t inViewTimestampUs = 0;
    uint64_t inUseTimestampUs = 0;
    std::vector<GPSSatelliteData> satellites = {};
    std::optional<std::set<int>> usedIds;
};

using SatelliteEpoch = std::vector<SatelliteSystem>;

struct GSV
{
    GPSConstellation constellation = GPSConstellation::Unknown;
    int messages = 0;
    int message = 0;
    int satelliteCount = 0;
    int signal = -1;
    std::vector<GPSSatelliteData> satellites = {};
};

inline std::optional<GSV> gsv(const Sentence& input)
{
    if (input.type() != "GSV" || input.count < 4)
        return {};
    const auto& f = input.fields;
    const auto total = NMEAFields::number<int>(f[1]);
    const auto message = NMEAFields::number<int>(f[2]);
    const auto count = NMEAFields::number<int>(f[3]);
    const auto system = satelliteConstellation(input.talker(), {}, {});
    if (!total || !message || !count || system == GPSConstellation::Unknown || *total < 1 || *total > 64 ||
        *message < 1 || *message > *total || *count < 0 || *count > 256 || *total != std::max(1, (*count + 3) / 4))
        return {};
    const int entries = std::min(4, *count - (*message - 1) * 4);
    const size_t end = 4 + entries * 4;
    if (input.count != end && input.count != end + 1)
        return {};
    GSV result{system, *total, *message, *count};
    if (input.count == end + 1 && !f[end].empty()) {
        const auto signal = NMEAFields::number<int>(f[end], 16);
        if (!signal || *signal < 0 || *signal > 15)
            return {};
        result.signal = *signal;
    }
    for (size_t index = 4; index < end; index += 4) {
        const auto id = NMEAFields::number<int>(f[index]);
        if (!id || *id < 1 || *id > 999)
            return {};
        GPSSatelliteData satellite;
        satellite.constellation = satelliteConstellation(input.talker(), {}, *id);
        satellite.id = gpsSatelliteId(satellite.constellation, *id);
        satellite.prn = *id;
        if (const auto value = NMEAFields::number<double>(f[index + 1]); value && *value >= 0 && *value <= 90)
            satellite.elevation = value;
        if (const auto value = NMEAFields::number<double>(f[index + 2]); value && *value >= 0 && *value <= 360)
            satellite.azimuth = value;
        if (const auto value = NMEAFields::number<int>(f[index + 3]); value && *value >= 0 && *value <= 99)
            satellite.signal = value;
        result.satellites.push_back(satellite);
    }
    return result;
}

/// Bounded multipart/multisignal assembly; scheduling and delivery remain with the caller.
class SatelliteAssembler
{
public:
    struct Update
    {
        bool accepted = false;
        SatelliteEpoch completed;
    };

    void clear()
    {
        _views.clear();
        _used.clear();
        _time.clear();
    }

    Update ingest(const Sentence& input, uint64_t receivedAtUs)
    {
        Update update;
        if ((input.type() == "RMC" || input.type() == "GGA") && input.count > 1 && !input.fields[1].empty()) {
            if (input.fields[1] != _time) {
                update.completed = flush();
                _time = input.fields[1];
            }
            return update;
        }
        if (input.type() == "GSV") {
            const auto parsed = gsv(input);
            if (!parsed)
                return update;
            const auto& page = *parsed;
            auto& signalReports = _views[page.constellation];
            if (page.message == 1 && signalReports[page.signal].complete())
                update.completed = flush();
            auto& report = _views[page.constellation][page.signal];
            if (page.message == 1)
                report = {page.messages, page.satelliteCount, 1, receivedAtUs, {}};
            if (report.messages != page.messages || report.count != page.satelliteCount ||
                report.next != page.message) {
                _views[page.constellation].erase(page.signal);
                return update;
            }
            report.satellites.insert(report.satellites.end(), page.satellites.begin(), page.satellites.end());
            report.timestamp = std::min(report.timestamp, receivedAtUs);
            ++report.next;
            update.accepted = true;
        } else if (input.type() == "GSA" && input.count >= 18) {
            const auto& f = input.fields;
            const auto fix = NMEAFields::number<int>(f[2]);
            if (!fix || *fix < 1 || *fix > 3)
                return update;
            std::optional<int> system;
            if (input.talker() == "GN" && input.count > 18 && !f[18].empty()) {
                system = NMEAFields::number<int>(f[18], 16);
                if (!system)
                    return update;
            }
            std::map<GPSConstellation, Used> reports;
            const auto explicitSystem = satelliteConstellation(input.talker(), system, {});
            if (explicitSystem != GPSConstellation::Unknown)
                reports[explicitSystem].timestamp = receivedAtUs;
            if (explicitSystem == GPSConstellation::GPS)
                reports[GPSConstellation::SBAS].timestamp = receivedAtUs;
            for (size_t index = 3; index < 15; ++index) {
                if (f[index].empty())
                    continue;
                const auto id = NMEAFields::number<int>(f[index]);
                if (!id || *id <= 0)
                    return update;
                const auto constellation = satelliteConstellation(input.talker(), system, id);
                if (constellation == GPSConstellation::Unknown)
                    return update;
                auto& report = reports[constellation];
                report.timestamp = receivedAtUs;
                if (*fix != 1)
                    report.ids.insert(gpsSatelliteId(constellation, *id));
            }
            if (reports.empty())
                return update;
            if (!_views.empty() && std::any_of(reports.begin(), reports.end(),
                                               [this](const auto& report) { return _used.contains(report.first); }))
                update.completed = flush();
            for (auto& [constellation, report] : reports)
                _used[constellation] = std::move(report);
            update.accepted = true;
        }
        return update;
    }

    SatelliteEpoch flush()
    {
        std::map<GPSConstellation, SatelliteSystem> systems;
        for (const auto& [system, signalReports] : _views) {
            std::map<std::pair<GPSConstellation, int>, GPSSatelliteData> satellites;
            for (const auto& [signal, report] : signalReports) {
                if (!report.complete())
                    continue;
                auto& out = systems[system];
                out.constellation = system;
                out.inViewTimestampUs =
                    out.inViewTimestampUs ? std::min(out.inViewTimestampUs, report.timestamp) : report.timestamp;
                if (system == GPSConstellation::GPS) {
                    auto& sbas = systems[GPSConstellation::SBAS];
                    sbas.constellation = GPSConstellation::SBAS;
                    sbas.inViewTimestampUs = out.inViewTimestampUs;
                }
                for (const auto& satellite : report.satellites) {
                    const auto key = std::make_pair(satellite.constellation, satellite.id);
                    const auto existing = satellites.find(key);
                    if (existing == satellites.end() ||
                        satellite.signal.value_or(-1) > existing->second.signal.value_or(-1))
                        satellites[key] = satellite;
                }
            }
            for (auto& [id, satellite] : satellites) {
                auto& out = systems[satellite.constellation];
                out.constellation = satellite.constellation;
                const auto timestamp = systems[system].inViewTimestampUs;
                out.inViewTimestampUs = out.inViewTimestampUs ? std::min(out.inViewTimestampUs, timestamp) : timestamp;
                out.satellites.push_back(satellite);
            }
        }
        for (const auto& [system, used] : _used) {
            auto& out = systems[system];
            out.constellation = system;
            out.inUseTimestampUs = used.timestamp;
            out.usedIds = used.ids;
        }
        SatelliteEpoch result;
        for (auto& [system, report] : systems)
            result.push_back(std::move(report));
        _views.clear();
        _used.clear();
        return result;
    }

private:
    struct View
    {
        int messages = 0;
        int count = 0;
        int next = 1;
        uint64_t timestamp = 0;
        std::vector<GPSSatelliteData> satellites = {};

        bool complete() const { return messages > 0 && next == messages + 1; }
    };

    struct Used
    {
        uint64_t timestamp = 0;
        std::set<int> ids;
    };

    std::map<GPSConstellation, std::map<int, View>> _views;
    std::map<GPSConstellation, Used> _used;
    std::string _time;
};
}  // namespace NMEA
