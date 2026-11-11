#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "GPSCommand.h"
#include "NMEASentence.h"

/// PQTM reply fields and the acknowledgement and readback rules every command shares.
namespace QuectelCodec {

/// Where a reply carries its fields. A setting answers "<name>,OK[,<values>]" or "<name>,ERROR,<code>".
namespace Field {
inline constexpr size_t NAME = 0;
inline constexpr size_t STATUS = 1;
inline constexpr size_t REJECTION_CODE = 2;
inline constexpr size_t ACKNOWLEDGEMENT_FIELDS = STATUS + 1;
inline constexpr size_t REJECTION_FIELDS = REJECTION_CODE + 1;
/// "PQTMVERNO,<firmware>,<date>,<time>"
inline constexpr size_t IDENTITY_FIRMWARE = 1;
inline constexpr size_t IDENTITY_FIELDS = 4;
/// The boot banner "PQTMVER,<version>,MODULE,<firmware>,<date>,<time>"
inline constexpr size_t BOOT_VERSION = 1;
inline constexpr size_t BOOT_TYPE = 2;
inline constexpr size_t BOOT_FIRMWARE = 3;
inline constexpr size_t BOOT_FIELDS = 6;
/// "PQTMCFGRCVRMODE,OK,<role>"
inline constexpr size_t ROLE = 2;
inline constexpr size_t ROLE_FIELDS = ROLE + 1;
/// "PQTMCFGFIXRATE,OK,<milliseconds>"
inline constexpr size_t FIX_INTERVAL = 2;
inline constexpr size_t FIX_RATE_FIELDS = FIX_INTERVAL + 1;
/// "PQTMCFGMSGRATE,OK,<message>,<rate>[,<version>]"
inline constexpr size_t RATE_MESSAGE = 2;
inline constexpr size_t RATE_VALUE = 3;
inline constexpr size_t RATE_VERSION = 4;
inline constexpr size_t RATE_MIN_FIELDS = RATE_VALUE + 1;
inline constexpr size_t RATE_MAX_FIELDS = RATE_VERSION + 1;
/// "PQTMCFGSVIN,OK,<mode>,<observations>,<accuracy>,<x>,<y>,<z>[,<distance>]"
inline constexpr size_t BASE_MODE = 2;
inline constexpr size_t BASE_OBSERVATIONS = 3;
inline constexpr size_t BASE_ACCURACY = 4;
inline constexpr size_t BASE_X = 5;
inline constexpr size_t BASE_Y = 6;
inline constexpr size_t BASE_Z = 7;
inline constexpr size_t BASE_DISTANCE = 8;
inline constexpr size_t BASE_MIN_FIELDS = BASE_Z + 1;
inline constexpr size_t BASE_MAX_FIELDS = BASE_DISTANCE + 1;
/// "PQTMSVINSTATUS,<version>,<time of week>,<validity>,<reserved>,<unread>,<observations>,<configured
/// observations>,<x>,<y>,<z>,<accuracy>"
inline constexpr size_t SURVEY_VERSION = 1;
inline constexpr size_t SURVEY_TIME_OF_WEEK = 2;
inline constexpr size_t SURVEY_VALIDITY = 3;
inline constexpr size_t SURVEY_RESERVED = 4;
inline constexpr size_t SURVEY_OBSERVATIONS = 6;
inline constexpr size_t SURVEY_CONFIGURED_OBSERVATIONS = 7;
inline constexpr size_t SURVEY_X = 8;
inline constexpr size_t SURVEY_Y = 9;
inline constexpr size_t SURVEY_Z = 10;
inline constexpr size_t SURVEY_ACCURACY = 11;
inline constexpr size_t SURVEY_FIELDS = SURVEY_ACCURACY + 1;
}  // namespace Field

inline constexpr std::string_view REPLY_OK = "OK";
inline constexpr std::string_view REPLY_ERROR = "ERROR";

/// Borrowed fields for the largest supported PQTM reply, including empty trailing fields.
class Fields
{
public:
    /// The survey status is the longest reply read.
    static constexpr size_t MAX_FIELDS = Field::SURVEY_FIELDS;

    explicit Fields(std::string_view body)
        : _count(NMEA::splitFields(body, _fields))
        , _overflowed(_count == 0)
    {}

    size_t size() const { return _count; }

    bool overflowed() const { return _overflowed; }

    std::string_view operator[](size_t index) const { return _fields[index]; }

    std::string_view back() const { return _count ? _fields[_count - 1] : std::string_view{}; }

    void pop_back()
    {
        if (_count) {
            --_count;
        }
    }

private:
    std::array<std::string_view, MAX_FIELDS> _fields{};
    size_t _count;
    bool _overflowed;
};

template <typename T>
bool number(std::string_view text, T& result)
{
    if (text.empty() || text.front() == '+') {
        return false;
    }
    const auto parsed = NMEA::number<T>(text);
    if (!parsed) {
        return false;
    }
    result = *parsed;
    return true;
}

/// The body of a checksum-valid "$<body>*hh" line; empty otherwise.
[[nodiscard]] inline std::string_view checkedBody(std::string_view line)
{
    const auto wire = NMEA::frame(line);
    return wire && wire->hasValidChecksum() ? wire->body : std::string_view{};
}

[[nodiscard]] inline bool rejected(const Fields& reply, std::string_view command)
{
    unsigned error = 0;
    return reply.size() == Field::REJECTION_FIELDS && reply[Field::NAME] == command &&
           reply[Field::STATUS] == REPLY_ERROR && number(reply[Field::REJECTION_CODE], error);
}

/// "<command>,OK" acknowledges and "<command>,ERROR,<code>" rejects a setting.
[[nodiscard]] inline GPSCommandOutcome acknowledgement(const Fields& reply, std::string_view command)
{
    if (reply.size() == Field::ACKNOWLEDGEMENT_FIELDS && reply[Field::NAME] == command &&
        reply[Field::STATUS] == REPLY_OK) {
        return GPSCommandOutcome::Acknowledged;
    }
    return rejected(reply, command) ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Pending;
}

[[nodiscard]] inline GPSCommandOutcome readback(const Fields& reply, std::string_view command, bool matches)
{
    if (rejected(reply, command)) {
        return GPSCommandOutcome::Rejected;
    }
    // Overflow retains the header, but can never verify a truncated readback.
    if ((reply.size() > Field::ACKNOWLEDGEMENT_FIELDS || reply.overflowed()) && reply[Field::NAME] == command &&
        reply[Field::STATUS] == REPLY_OK) {
        return !reply.overflowed() && matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    }
    return GPSCommandOutcome::Pending;
}

}  // namespace QuectelCodec
