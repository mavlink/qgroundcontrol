#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "GPSCommand.h"
#include "NMEASentence.h"

/// PQTM reply fields and the acknowledgement and readback rules every command shares.
namespace QuectelCodec {

/// Borrowed fields for the largest supported PQTM reply, including empty trailing fields.
class Fields
{
public:
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
    std::array<std::string_view, 12> _fields{};
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
    return reply.size() == 3 && reply[0] == command && reply[1] == "ERROR" && number(reply[2], error);
}

/// "<command>,OK" acknowledges and "<command>,ERROR,<code>" rejects a setting.
[[nodiscard]] inline GPSCommandOutcome acknowledgement(const Fields& reply, std::string_view command)
{
    if (reply.size() == 2 && reply[0] == command && reply[1] == "OK") {
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
    if ((reply.size() > 2 || reply.overflowed()) && reply[0] == command && reply[1] == "OK") {
        return !reply.overflowed() && matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    }
    return GPSCommandOutcome::Pending;
}

}  // namespace QuectelCodec
