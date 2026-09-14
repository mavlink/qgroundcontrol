#include "NMEASatelliteEpoch.h"

#include <algorithm>
#include <utility>

#include "NMEAConstellation.h"
#include "NMEASentence.h"

namespace {
constexpr size_t GSV_PAGE_COUNT_FIELD = 1;
constexpr size_t GSV_PAGE_NUMBER_FIELD = 2;
constexpr size_t GSV_SATELLITE_COUNT_FIELD = 3;
constexpr size_t GSV_FIRST_SATELLITE_FIELD = 4;
constexpr size_t GSV_SATELLITE_FIELDS = 4;
constexpr int GSV_SATELLITES_PER_PAGE = 4;
constexpr int MAX_GSV_PAGES = 64;
constexpr int MAX_GSV_SATELLITES = MAX_GSV_PAGES * GSV_SATELLITES_PER_PAGE;
constexpr int MAX_GSV_SIGNAL_ID = 15;
constexpr int MAX_GSV_SATELLITE_ID = 999;
constexpr size_t GSV_ELEVATION_OFFSET = 1;
constexpr size_t GSV_AZIMUTH_OFFSET = 2;
constexpr size_t GSV_SIGNAL_OFFSET = 3;
constexpr double MAX_ELEVATION_DEGREES = 90.0;
constexpr double MAX_AZIMUTH_DEGREES = 360.0;
constexpr int MAX_SIGNAL_STRENGTH = 99;
}  // namespace

namespace NMEA {
std::optional<GSV> gsv(const Sentence& input)
{
    if (input.type() != "GSV" || input.count < GSV_FIRST_SATELLITE_FIELD)
        return {};
    const auto& f = input.fields;
    const auto total = number<int>(f[GSV_PAGE_COUNT_FIELD]);
    const auto message = number<int>(f[GSV_PAGE_NUMBER_FIELD]);
    const auto count = number<int>(f[GSV_SATELLITE_COUNT_FIELD]);
    const auto system = satelliteConstellation(input.talker(), {}, {});
    if (!total || !message || !count || (system == GPSConstellation::Unknown && input.talker() != "GN") || *total < 1 ||
        *total > MAX_GSV_PAGES || *message < 1 || *message > *total || *count < 0 || *count > MAX_GSV_SATELLITES ||
        *total != std::max(1, (*count + GSV_SATELLITES_PER_PAGE - 1) / GSV_SATELLITES_PER_PAGE))
        return {};
    const int entries = std::min(GSV_SATELLITES_PER_PAGE, *count - (*message - 1) * GSV_SATELLITES_PER_PAGE);
    const size_t end = GSV_FIRST_SATELLITE_FIELD + entries * GSV_SATELLITE_FIELDS;
    const size_t fields = input.count - GSV_FIRST_SATELLITE_FIELD;
    const size_t slots = fields / GSV_SATELLITE_FIELDS;
    const size_t trailingFields = fields % GSV_SATELLITE_FIELDS;
    if (slots < static_cast<size_t>(entries) || slots > GSV_SATELLITES_PER_PAGE || trailingFields > 1)
        return {};
    const size_t signalIndex = GSV_FIRST_SATELLITE_FIELD + slots * GSV_SATELLITE_FIELDS;
    // Some receivers retain all four satellite slots on short or empty pages.
    for (size_t index = end; index < signalIndex; ++index) {
        if (!f[index].empty())
            return {};
    }
    GSV result{system, *total, *message, *count};
    if (trailingFields == 1 && !f[signalIndex].empty()) {
        const auto signal = number<int>(f[signalIndex], HEX_BASE);
        if (!signal || *signal < 0 || *signal > MAX_GSV_SIGNAL_ID)
            return {};
        result.signal = *signal;
    }
    for (size_t index = GSV_FIRST_SATELLITE_FIELD; index < end; index += GSV_SATELLITE_FIELDS) {
        const auto id = number<int>(f[index]);
        if (!id || *id < 1 || *id > MAX_GSV_SATELLITE_ID)
            return {};
        SatelliteData satellite;
        satellite.constellation = satelliteConstellation(input.talker(), {}, *id);
        if (satellite.constellation == GPSConstellation::Unknown)
            return {};
        satellite.id = gpsSatelliteId(satellite.constellation, *id);
        satellite.prn = *id;
        if (const auto value = number<double>(f[index + GSV_ELEVATION_OFFSET]);
            value && *value >= 0 && *value <= MAX_ELEVATION_DEGREES)
            satellite.elevation = value;
        if (const auto value = number<double>(f[index + GSV_AZIMUTH_OFFSET]);
            value && *value >= 0 && *value <= MAX_AZIMUTH_DEGREES)
            satellite.azimuth = value;
        if (const auto value = number<int>(f[index + GSV_SIGNAL_OFFSET]);
            value && *value >= 0 && *value <= MAX_SIGNAL_STRENGTH)
            satellite.signal = value;
        result.satellites.push_back(satellite);
    }
    return result;
}

void SatelliteAssembler::clear()
{
    _views.clear();
    _used.clear();
    _time.reset();
}

SatelliteAssembler::Update SatelliteAssembler::ingest(const Sentence& input, uint64_t receivedAtUs)
{
    Update update;
    if ((input.type() == "RMC" || input.type() == "GGA") && input.count > Field::UTC_TIME) {
        const auto time = utcMilliseconds(input.fields[Field::UTC_TIME]);
        if (time && time != _time) {
            update.completed = flush();
            _time = time;
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
        if (report.messages != page.messages || report.count != page.satelliteCount || report.next != page.message) {
            _views[page.constellation].erase(page.signal);
            return update;
        }
        report.satellites.insert(report.satellites.end(), page.satellites.begin(), page.satellites.end());
        report.timestamp = std::min(report.timestamp, receivedAtUs);
        ++report.next;
        update.accepted = true;
    } else if (input.type() == "GSA" && input.count >= Field::GSA_MIN_FIELDS) {
        const auto& f = input.fields;
        const auto fix = number<unsigned>(f[Field::GSA_DIMENSION]);
        if (!fix || *fix < FixDimension::NO_FIX || *fix > FixDimension::THREE_D)
            return update;
        std::optional<int> system;
        if (input.talker() == "GN" && input.count > Field::GSA_SYSTEM_ID && !f[Field::GSA_SYSTEM_ID].empty()) {
            system = number<int>(f[Field::GSA_SYSTEM_ID], HEX_BASE);
            if (!system)
                return update;
        }
        std::map<GPSConstellation, Used> reports;
        const auto explicitSystem = satelliteConstellation(input.talker(), system, {});
        if (explicitSystem != GPSConstellation::Unknown)
            reports[explicitSystem].timestamp = receivedAtUs;
        if (explicitSystem == GPSConstellation::GPS)
            reports[GPSConstellation::SBAS].timestamp = receivedAtUs;
        for (size_t index = Field::GSA_FIRST_SATELLITE; index < Field::GSA_FIRST_SATELLITE + Field::GSA_SATELLITE_SLOTS;
             ++index) {
            if (f[index].empty())
                continue;
            const auto id = number<int>(f[index]);
            if (!id || *id <= 0)
                return update;
            const auto constellation = satelliteConstellation(input.talker(), system, id);
            if (constellation == GPSConstellation::Unknown)
                return update;
            auto& report = reports[constellation];
            report.timestamp = receivedAtUs;
            if (*fix != FixDimension::NO_FIX)
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

SatelliteEpoch SatelliteAssembler::flush()
{
    std::map<GPSConstellation, SatelliteSystem> systems;
    std::map<GPSConstellation, std::map<int, SatelliteData>> satellites;
    const auto includeView = [&systems](GPSConstellation constellation, uint64_t timestamp) {
        auto& out = systems[constellation];
        out.constellation = constellation;
        out.inViewTimestampUs = out.inViewTimestampUs ? std::min(out.inViewTimestampUs, timestamp) : timestamp;
    };
    for (const auto& [system, signalReports] : _views) {
        for (const auto& [signal, report] : signalReports) {
            if (!report.complete())
                continue;
            // GN identifies coverage only through its entries, including across pages and signals.
            if (system != GPSConstellation::Unknown) {
                includeView(system, report.timestamp);
                if (system == GPSConstellation::GPS)
                    includeView(GPSConstellation::SBAS, report.timestamp);
            }
            for (const auto& satellite : report.satellites) {
                includeView(satellite.constellation, report.timestamp);
                auto& view = satellites[satellite.constellation];
                const auto existing = view.find(satellite.id);
                if (existing == view.end() || satellite.signal.value_or(-1) > existing->second.signal.value_or(-1))
                    view[satellite.id] = satellite;
            }
        }
    }
    for (auto& [system, view] : satellites) {
        for (auto& [id, satellite] : view)
            systems[system].satellites.push_back(std::move(satellite));
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
}  // namespace NMEA
