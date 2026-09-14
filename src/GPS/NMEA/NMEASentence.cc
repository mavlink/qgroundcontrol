#include "NMEASentence.h"

#include <chrono>
#include <cstdint>
#include <limits>

namespace {
constexpr double DEGREE_MINUTE_SCALE = 100.0;
constexpr double MAX_LATITUDE_DEGREES = 90.0;
constexpr double MAX_LONGITUDE_DEGREES = 180.0;
constexpr unsigned MAX_GGA_SATELLITES = std::numeric_limits<uint8_t>::max();
constexpr size_t UTC_COMPONENT_DIGITS = 2;
constexpr size_t UTC_INTEGER_DIGITS = 3 * UTC_COMPONENT_DIGITS;
constexpr size_t UTC_FRACTION_OFFSET = UTC_INTEGER_DIGITS + 1;
constexpr unsigned HOURS_PER_DAY = 24;
constexpr unsigned MINUTES_PER_HOUR = 60;
constexpr unsigned SECONDS_PER_MINUTE = 60;
constexpr size_t FIRST_MILLISECOND_DIGIT_WEIGHT = 100;
}  // namespace

namespace NMEA {
unsigned char checksum(std::string_view body)
{
    unsigned char result = 0;
    for (const char byte : body)
        result ^= static_cast<unsigned char>(byte);
    return result;
}

bool Frame::hasValidChecksum() const
{
    if (checksum.size() != CHECKSUM_DIGITS)
        return false;
    unsigned expected = 0;
    const auto result = std::from_chars(checksum.data(), checksum.data() + checksum.size(), expected, HEX_BASE);
    return result.ec == std::errc{} && result.ptr == checksum.data() + checksum.size() &&
           expected == NMEA::checksum(body);
}

std::optional<Frame> frame(std::string_view text)
{
    if (text.ends_with('\n')) {
        text.remove_suffix(1);
        if (text.ends_with('\r'))
            text.remove_suffix(1);
    }
    if (text.empty() || text.front() != '$')
        return {};
    text.remove_prefix(Sentence::PREFIX_LENGTH);
    const auto star = text.find('*');
    const auto body = text.substr(0, star);
    if (body.empty())
        return {};
    for (char byte : body) {
        if (byte < ' ' || byte > '~' || byte == '$')
            return {};
    }
    return Frame{body, star == std::string_view::npos ? std::string_view() : text.substr(star + 1)};
}

std::optional<Sentence> sentence(std::string_view text)
{
    const auto wire = frame(text);
    if (!wire || !wire->hasValidChecksum())
        return {};
    Sentence result;
    text = text.substr(0, Sentence::PREFIX_LENGTH + wire->body.size());
    do {
        if (result.count == result.fields.size())
            return {};
        const auto comma = text.find(',');
        result.fields[result.count++] = text.substr(0, comma);
        if (comma == std::string_view::npos)
            break;
        text.remove_prefix(comma + 1);
    } while (true);
    if (result.fields[0].size() != Sentence::HEADER_LENGTH)
        return {};
    return result;
}

std::optional<double> coordinate(std::string_view field, std::string_view hemisphere, bool latitude)
{
    const auto value = number<double>(field);
    if (!value || *value < 0 || hemisphere.size() != 1)
        return {};
    const double degrees = std::trunc(*value / DEGREE_MINUTE_SCALE);
    const double minutes = *value - degrees * DEGREE_MINUTE_SCALE;
    const double limit = latitude ? MAX_LATITUDE_DEGREES : MAX_LONGITUDE_DEGREES;
    if (minutes >= MINUTES_PER_DEGREE || degrees + minutes / MINUTES_PER_DEGREE > limit)
        return {};
    const char positive = latitude ? 'N' : 'E';
    const char negative = latitude ? 'S' : 'W';
    if (hemisphere[0] != positive && hemisphere[0] != negative)
        return {};
    return (degrees + minutes / MINUTES_PER_DEGREE) * (hemisphere[0] == negative ? -1 : 1);
}

std::optional<GGA> gga(const Sentence& input)
{
    if (input.type() != "GGA" || input.count < Field::GGA_MIN_FIELDS)
        return {};
    const auto& f = input.fields;
    const auto latitude = coordinate(f[Field::GGA_LATITUDE], f[Field::GGA_LATITUDE_HEMISPHERE], true);
    const auto longitude = coordinate(f[Field::GGA_LONGITUDE], f[Field::GGA_LONGITUDE_HEMISPHERE], false);
    const auto quality = number<unsigned>(f[Field::GGA_QUALITY]);
    const auto satellites = number<unsigned>(f[Field::GGA_SATELLITES_USED]);
    if (!latitude || !longitude || !quality || *quality > GgaQuality::MAX_VALUE ||
        (satellites && *satellites > MAX_GGA_SATELLITES))
        return {};
    GGA result{*latitude, *longitude};
    result.quality = *quality;
    result.satellitesUsed = satellites;
    result.hdop = number<double>(f[Field::GGA_HDOP]).value_or(NAN);
    if (f[Field::GGA_ALTITUDE_UNITS] == "M")
        result.altitude = number<double>(f[Field::GGA_ALTITUDE]).value_or(NAN);
    if (f[Field::GGA_GEOID_UNITS] == "M")
        result.geoidSeparation = number<double>(f[Field::GGA_GEOID_SEPARATION]).value_or(NAN);
    return result;
}

std::optional<int> utcMilliseconds(std::string_view field)
{
    if (field.size() < UTC_INTEGER_DIGITS ||
        (field.size() > UTC_INTEGER_DIGITS &&
         (field[UTC_INTEGER_DIGITS] != '.' || field.size() == UTC_FRACTION_OFFSET)))
        return {};
    for (size_t index = 0; index < field.size(); ++index) {
        if (index != UTC_INTEGER_DIGITS && (field[index] < '0' || field[index] > '9'))
            return {};
    }
    const auto hours = number<unsigned>(field.substr(0, UTC_COMPONENT_DIGITS));
    const auto minutes = number<unsigned>(field.substr(UTC_COMPONENT_DIGITS, UTC_COMPONENT_DIGITS));
    const auto seconds = number<unsigned>(field.substr(2 * UTC_COMPONENT_DIGITS, UTC_COMPONENT_DIGITS));
    if (!hours || !minutes || !seconds || *hours >= HOURS_PER_DAY || *minutes >= MINUTES_PER_HOUR ||
        *seconds >= SECONDS_PER_MINUTE)
        return {};
    int milliseconds = 0;
    // Truncate sub-millisecond precision without floating-point rounding into the preceding epoch.
    for (size_t index = UTC_FRACTION_OFFSET, scale = FIRST_MILLISECOND_DIGIT_WEIGHT; index < field.size() && scale > 0;
         ++index, scale /= DECIMAL_BASE)
        milliseconds += static_cast<int>((field[index] - '0') * scale);
    const auto wholeSeconds =
        std::chrono::hours(*hours) + std::chrono::minutes(*minutes) + std::chrono::seconds(*seconds);
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(wholeSeconds).count()) + milliseconds;
}

std::optional<GST> gst(const Sentence& input)
{
    if (input.type() != "GST" || input.count != Field::GST_FIELDS)
        return {};
    const auto latitude = number<double>(input.fields[Field::GST_LATITUDE_ERROR]);
    const auto longitude = number<double>(input.fields[Field::GST_LONGITUDE_ERROR]);
    const auto altitude = number<double>(input.fields[Field::GST_ALTITUDE_ERROR]);
    GST result;
    if (latitude && longitude && *latitude >= 0 && *longitude >= 0)
        result.horizontalAccuracy = std::hypot(*latitude, *longitude);
    if (altitude && *altitude >= 0)
        result.verticalAccuracy = *altitude;
    return result;
}
}  // namespace NMEA
