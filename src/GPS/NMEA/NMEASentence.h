#pragma once

#include <array>
#include <cmath>
#include <optional>
#include <string_view>

#include "NMEAFields.h"

namespace NMEA {
struct Sentence
{
    std::array<std::string_view, 32> fields{};
    size_t count = 0;

    std::string_view type() const { return fields[0].substr(3); }

    std::string_view talker() const { return fields[0].substr(1, 2); }
};

inline std::optional<Sentence> sentence(std::string_view text)
{
    const auto star = text.find('*');
    if (text.empty() || text.front() != '$' || star == std::string_view::npos || star + 3 > text.size())
        return {};
    unsigned checksum = 0;
    for (size_t index = 1; index < star; ++index)
        checksum ^= static_cast<unsigned char>(text[index]);
    const auto expected = NMEAFields::number<unsigned>(text.substr(star + 1, 2), 16);
    if (!expected || *expected != checksum)
        return {};
    Sentence result;
    text = text.substr(0, star);
    do {
        if (result.count == result.fields.size())
            return {};
        const auto comma = text.find(',');
        result.fields[result.count++] = text.substr(0, comma);
        if (comma == std::string_view::npos)
            break;
        text.remove_prefix(comma + 1);
    } while (true);
    if (result.fields[0].size() != 6)
        return {};
    return result;
}

inline std::optional<double> coordinate(std::string_view field, std::string_view hemisphere, bool latitude)
{
    const auto value = NMEAFields::number<double>(field);
    if (!value || *value < 0 || hemisphere.size() != 1)
        return {};
    const double degrees = std::trunc(*value / 100);
    const double minutes = *value - degrees * 100;
    const double limit = latitude ? 90 : 180;
    if (minutes >= 60 || degrees + minutes / 60 > limit)
        return {};
    const char positive = latitude ? 'N' : 'E';
    const char negative = latitude ? 'S' : 'W';
    if (hemisphere[0] != positive && hemisphere[0] != negative)
        return {};
    return (degrees + minutes / 60) * (hemisphere[0] == negative ? -1 : 1);
}

struct GGA
{
    double latitude = NAN;
    double longitude = NAN;
    double altitude = NAN;
    double geoidSeparation = NAN;
    double hdop = NAN;
    unsigned quality = 0;
    std::optional<unsigned> satellitesUsed = std::nullopt;
};

inline std::optional<GGA> gga(const Sentence& input)
{
    if (input.type() != "GGA" || input.count < 13)
        return {};
    const auto& f = input.fields;
    const auto latitude = coordinate(f[2], f[3], true);
    const auto longitude = coordinate(f[4], f[5], false);
    const auto quality = NMEAFields::number<unsigned>(f[6]);
    const auto satellites = NMEAFields::number<unsigned>(f[7]);
    if (!latitude || !longitude || !quality || *quality > 8 || (satellites && *satellites > 255))
        return {};
    GGA result{*latitude, *longitude};
    result.quality = *quality;
    result.satellitesUsed = satellites;
    result.hdop = NMEAFields::number<double>(f[8]).value_or(NAN);
    if (f[10] == "M")
        result.altitude = NMEAFields::number<double>(f[9]).value_or(NAN);
    if (f[12] == "M")
        result.geoidSeparation = NMEAFields::number<double>(f[11]).value_or(NAN);
    return result;
}

struct GST
{
    double horizontalAccuracy = NAN;
    double verticalAccuracy = NAN;
};

inline std::optional<GST> gst(const Sentence& input)
{
    if (input.type() != "GST" || input.count != 9)
        return {};
    const auto latitude = NMEAFields::number<double>(input.fields[6]);
    const auto longitude = NMEAFields::number<double>(input.fields[7]);
    const auto altitude = NMEAFields::number<double>(input.fields[8]);
    GST result;
    if (latitude && longitude && *latitude >= 0 && *longitude >= 0)
        result.horizontalAccuracy = std::hypot(*latitude, *longitude);
    if (altitude && *altitude >= 0)
        result.verticalAccuracy = *altitude;
    return result;
}
}  // namespace NMEA
