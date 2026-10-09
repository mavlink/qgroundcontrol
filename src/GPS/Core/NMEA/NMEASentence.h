#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QTime>

#include "GPSReceiverReports.h"

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
constexpr size_t RMC_LATITUDE = 3;
constexpr size_t RMC_LATITUDE_HEMISPHERE = 4;
constexpr size_t RMC_LONGITUDE = 5;
constexpr size_t RMC_LONGITUDE_HEMISPHERE = 6;
constexpr size_t RMC_SPEED_KNOTS = 7;
constexpr size_t RMC_COURSE = 8;
constexpr size_t RMC_DATE = 9;
constexpr size_t RMC_MIN_FIELDS = RMC_DATE + 1;
constexpr size_t GLL_LATITUDE = 1;
constexpr size_t GLL_LATITUDE_HEMISPHERE = 2;
constexpr size_t GLL_LONGITUDE = 3;
constexpr size_t GLL_LONGITUDE_HEMISPHERE = 4;
constexpr size_t GLL_TIME = 5;
constexpr size_t GLL_STATUS = 6;
constexpr size_t GLL_MIN_FIELDS = GLL_STATUS + 1;
constexpr size_t GSA_DIMENSION = 2;
constexpr size_t GSA_FIRST_SATELLITE = 3;
constexpr size_t GSA_SATELLITE_SLOTS = 12;
constexpr size_t GSA_HDOP = GSA_FIRST_SATELLITE + GSA_SATELLITE_SLOTS + 1;
constexpr size_t GSA_VDOP = GSA_HDOP + 1;
constexpr size_t GSA_MIN_FIELDS = GSA_VDOP + 1;
constexpr size_t GSA_SYSTEM_ID = GSA_MIN_FIELDS;
constexpr size_t VTG_TRUE_COURSE = 1;
constexpr size_t VTG_SPEED_KNOTS = 5;
constexpr size_t VTG_SPEED_KMH = 7;
constexpr size_t VTG_MIN_FIELDS = VTG_SPEED_KMH + 1;
constexpr size_t ZDA_DAY = 2;
constexpr size_t ZDA_MONTH = 3;
constexpr size_t ZDA_YEAR = 4;
constexpr size_t ZDA_MIN_FIELDS = ZDA_YEAR + 1;
constexpr size_t GST_LATITUDE_ERROR = 6;
constexpr size_t GST_LONGITUDE_ERROR = 7;
constexpr size_t GST_ALTITUDE_ERROR = 8;
}  // namespace Field

namespace GgaQuality {
constexpr unsigned INVALID = 0;
constexpr unsigned GPS = 1;
constexpr unsigned DIFFERENTIAL = 2;
constexpr unsigned RTK_FIXED = 4;
constexpr unsigned RTK_FLOAT = 5;
constexpr unsigned ESTIMATED = 6;
/// Manual input: the position a base station was fixed at.
constexpr unsigned MANUAL = 7;
constexpr unsigned MAX_VALUE = 8;
}  // namespace GgaQuality

namespace FixDimension {
constexpr unsigned NO_FIX = 1;
constexpr unsigned THREE_D = 3;
}  // namespace FixDimension

/// Autonomous GGA does not distinguish 2D/3D; the caller supplies its independently resolved quality.
[[nodiscard]] GPSFixQuality fixQuality(unsigned quality, GPSFixQuality autonomous);

template <class T>
[[nodiscard]] std::optional<T> number(std::string_view field, int base = DECIMAL_BASE)
{
    if (field.empty()) {
        return std::nullopt;
    }
    if (field.front() == '+') {
        field.remove_prefix(1);
        if (!field.empty() && (field.front() == '-' || field.front() == '+')) {
            return std::nullopt;
        }
    }
    if (field.empty()) {
        return std::nullopt;
    }
    T value{};
    if constexpr (std::is_integral_v<T>) {
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value, base);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size()) {
            return std::nullopt;
        }
    } else {
        static_assert(std::is_same_v<T, double> || std::is_same_v<T, float>);
        // Qt parses floating point in the C locale on every platform; libc++ before 20 has no floating-point
        // from_chars. Qt skips surrounding whitespace, which a field must not carry.
        constexpr std::string_view WHITESPACE = " \t\n\v\f\r";
        if (WHITESPACE.find(field.front()) != std::string_view::npos ||
            WHITESPACE.find(field.back()) != std::string_view::npos) {
            return std::nullopt;
        }
        bool ok = false;
        if constexpr (std::is_same_v<T, float>) {
            value = QByteArrayView(field).toFloat(&ok);
        } else {
            value = QByteArrayView(field).toDouble(&ok);
        }
        if (!ok || !std::isfinite(value)) {
            return std::nullopt;
        }
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

[[nodiscard]] unsigned char checksum(std::string_view body);

/// Locale-independent ASCII character classes for receiver text.
[[nodiscard]] constexpr bool isAsciiDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

[[nodiscard]] constexpr bool isAsciiUpper(char ch)
{
    return ch >= 'A' && ch <= 'Z';
}

/// The value of @a digits when every one is a hexadecimal digit, with no sign or prefix; nullopt otherwise.
[[nodiscard]] std::optional<uint32_t> hexNumber(std::string_view digits);

/// Preserve empty fields, including the final field. Return zero on storage overflow.
size_t splitFields(std::string_view text, std::span<std::string_view> fields);

/// Views into a single wire frame; the caller owns the backing bytes.
struct Frame
{
    std::string_view body;
    std::string_view checksum;

    [[nodiscard]] bool hasValidChecksum() const;
};

/// Split a printable $BODY[*CHECKSUM] frame, accepting no terminator, LF, or CRLF.
/// Checksum syntax and value are checked separately so malformed checksums can be repaired.
[[nodiscard]] std::optional<Frame> frame(std::string_view text);

[[nodiscard]] std::optional<Sentence> sentence(std::string_view text);

/// A checked sentence from a standard talker, not a proprietary $P... sentence.
[[nodiscard]] bool isStandardSentence(std::string_view text);

/// Convert a signed ddmm.mmmm (or dddmm.mmmm) value; invalid values return NaN.
[[nodiscard]] double degreesFromDegreesMinutes(double ddmm);

[[nodiscard]] std::optional<double> coordinate(std::string_view field, std::string_view hemisphere, bool latitude);

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

/// Quality zero is a valid fix-loss observation with empty coordinates and optional hemisphere fields.
[[nodiscard]] std::optional<GGA> gga(const Sentence& input);

std::optional<int> utcMilliseconds(std::string_view field);

struct UtcDate
{
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
};

[[nodiscard]] std::optional<UtcDate> rmcDate(std::string_view field);

struct RMC
{
    double latitude = NAN;
    double longitude = NAN;
    std::optional<int> utcMilliseconds;
    double speedMetersPerSecond = NAN;
    double courseDegrees = NAN;
};

[[nodiscard]] std::optional<RMC> rmc(const Sentence& input);

struct GLL
{
    double latitude = NAN;
    double longitude = NAN;
    std::optional<int> utcMilliseconds;
};

[[nodiscard]] std::optional<GLL> gll(const Sentence& input);

struct VTG
{
    double speedMetersPerSecond = NAN;
    double courseDegrees = NAN;
};

[[nodiscard]] std::optional<VTG> vtg(const Sentence& input);

struct ZDA
{
    std::optional<int> utcMilliseconds;
    UtcDate date;
};

[[nodiscard]] std::optional<ZDA> zda(const Sentence& input);

struct NavigationStatus
{
    bool valid = false;
    std::optional<int> utcMilliseconds;
};

/// Receiver-declared validity, independent of coordinate availability. A valid flag alone is not a usable fix.
/// GSA is untimed; malformed/unsupported status fields return no observation.
[[nodiscard]] std::optional<NavigationStatus> navigationStatus(const Sentence& input);

struct GST
{
    double horizontalAccuracy = NAN;
    double verticalAccuracy = NAN;
};

[[nodiscard]] std::optional<GST> gst(const Sentence& input);
}  // namespace NMEA

namespace NMEAUtils {
/// @a body as one "$<body>*hh\r\n" sentence, with uppercase checksum digits.
[[nodiscard]] QByteArray frame(QByteArrayView body);

/// Rebuild a valid frame with its checksum and CRLF; short or malformed bodies are only terminated.
[[nodiscard]] QByteArray repairChecksum(const QByteArray& sentence);

/// Build GPGGA from explicit fix fields and UTC, truncated to whole seconds.
/// NaN altitude/geoid separation/HDOP and absent satellite count produce empty fields, not defaults.
/// Returns empty for invalid UTC, out-of-range/nonfinite coordinates, infinite measurements,
/// negative HDOP, or quality above the protocol maximum. Altitude is MSL; geoid separation is signed.
QByteArray makeGGA(const NMEA::GGA& fix, const QTime& utc);

}  // namespace NMEAUtils
