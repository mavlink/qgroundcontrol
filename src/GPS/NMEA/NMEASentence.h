#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace NMEA {
constexpr int DECIMAL_BASE = 10;
constexpr int HEX_BASE = 16;
constexpr int CHECKSUM_DIGITS = 2;
constexpr double MINUTES_PER_DEGREE = 60.0;

namespace Field {
constexpr size_t UTC_TIME = 1;
constexpr size_t GGA_LATITUDE = 2;
constexpr size_t GGA_LATITUDE_HEMISPHERE = 3;
constexpr size_t GGA_LONGITUDE = 4;
constexpr size_t GGA_LONGITUDE_HEMISPHERE = 5;
constexpr size_t GGA_QUALITY = 6;
constexpr size_t GGA_SATELLITES_USED = 7;
constexpr size_t GGA_HDOP = 8;
constexpr size_t GGA_ALTITUDE = 9;
constexpr size_t GGA_ALTITUDE_UNITS = 10;
constexpr size_t GGA_GEOID_SEPARATION = 11;
constexpr size_t GGA_GEOID_UNITS = 12;
constexpr size_t GGA_MIN_FIELDS = GGA_GEOID_UNITS + 1;
constexpr size_t RMC_STATUS = 2;
constexpr size_t GLL_TIME = 5;
constexpr size_t GLL_STATUS = 6;
constexpr size_t GSA_DIMENSION = 2;
constexpr size_t GSA_FIRST_SATELLITE = 3;
constexpr size_t GSA_SATELLITE_SLOTS = 12;
constexpr size_t GSA_HDOP = GSA_FIRST_SATELLITE + GSA_SATELLITE_SLOTS + 1;
constexpr size_t GSA_VDOP = GSA_HDOP + 1;
constexpr size_t GSA_MIN_FIELDS = GSA_VDOP + 1;
constexpr size_t GSA_SYSTEM_ID = GSA_MIN_FIELDS;
constexpr size_t GST_LATITUDE_ERROR = 6;
constexpr size_t GST_LONGITUDE_ERROR = 7;
constexpr size_t GST_ALTITUDE_ERROR = 8;
constexpr size_t GST_FIELDS = GST_ALTITUDE_ERROR + 1;
}  // namespace Field

namespace GgaQuality {
constexpr unsigned INVALID = 0;
constexpr unsigned GPS = 1;
constexpr unsigned DIFFERENTIAL = 2;
constexpr unsigned RTK_FIXED = 4;
constexpr unsigned RTK_FLOAT = 5;
constexpr unsigned ESTIMATED = 6;
constexpr unsigned MAX_VALUE = 8;
}  // namespace GgaQuality

namespace FixDimension {
constexpr unsigned NO_FIX = 1;
constexpr unsigned TWO_D = 2;
constexpr unsigned THREE_D = 3;
}  // namespace FixDimension

template <class T>
std::optional<T> number(std::string_view field, int base = DECIMAL_BASE)
{
    if (field.empty())
        return std::nullopt;
    if (field.front() == '+') {
        field.remove_prefix(1);
        if (!field.empty() && (field.front() == '-' || field.front() == '+'))
            return std::nullopt;
    }
    if (field.empty())
        return std::nullopt;
    T value{};
    if constexpr (std::is_integral_v<T>) {
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value, base);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
            return std::nullopt;
    } else if constexpr (requires { std::from_chars(field.data(), field.data() + field.size(), value); }) {
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
            return std::nullopt;
    } else {
        // Older libc++ versions lack floating-point from_chars. Keep parsing locale independent.
        std::istringstream stream{std::string(field)};
        stream.imbue(std::locale::classic());
        stream >> std::noskipws >> value;
        if (stream.fail() || stream.peek() != std::char_traits<char>::eof())
            return std::nullopt;
    }
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(value))
            return std::nullopt;
    }
    return value;
}

struct Sentence
{
    static constexpr size_t MAX_FIELDS = 32;
    static constexpr int PREFIX_LENGTH = 1;
    static constexpr int TALKER_LENGTH = 2;
    static constexpr int TYPE_LENGTH = 3;
    static constexpr int TYPE_OFFSET = PREFIX_LENGTH + TALKER_LENGTH;
    static constexpr int HEADER_LENGTH = TYPE_OFFSET + TYPE_LENGTH;

    std::array<std::string_view, MAX_FIELDS> fields{};
    size_t count = 0;

    std::string_view type() const { return fields[0].substr(TYPE_OFFSET); }

    std::string_view talker() const { return fields[0].substr(PREFIX_LENGTH, TALKER_LENGTH); }
};

unsigned char checksum(std::string_view body);

/// Views into a single wire frame; the caller owns the backing bytes.
struct Frame
{
    std::string_view body;
    std::string_view checksum;

    bool hasValidChecksum() const;
};

/// Split a printable $BODY[*CHECKSUM] frame, accepting no terminator, LF, or CRLF.
/// Checksum syntax and value are checked separately so malformed checksums can be repaired.
std::optional<Frame> frame(std::string_view text);

std::optional<Sentence> sentence(std::string_view text);

std::optional<double> coordinate(std::string_view field, std::string_view hemisphere, bool latitude);

struct GGA
{
    double latitude = NAN;
    double longitude = NAN;
    double altitude = NAN;
    double geoidSeparation = NAN;
    double hdop = NAN;
    unsigned quality = GgaQuality::INVALID;
    std::optional<unsigned> satellitesUsed = std::nullopt;
};

std::optional<GGA> gga(const Sentence& input);

std::optional<int> utcMilliseconds(std::string_view field);

struct GST
{
    double horizontalAccuracy = NAN;
    double verticalAccuracy = NAN;
};

std::optional<GST> gst(const Sentence& input);
}  // namespace NMEA
