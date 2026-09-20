#include "NMEA/NMEAFields.h"
#include "QuectelCodec_p.h"

namespace QuectelCodec {

std::string frame(std::string_view body)
{
    const auto checksum = NMEA::checksum(body);
    std::string result{"$"};
    result.append(body);
    result.push_back('*');
    result.push_back(NMEAFields::hexDigit(checksum >> 4));
    result.push_back(NMEAFields::hexDigit(checksum));
    result.append("\r\n");
    return result;
}

std::string_view checkedBody(std::string_view line)
{
    const auto wire = NMEA::frame(line);
    return wire && wire->hasValidChecksum() ? wire->body : std::string_view{};
}

bool rejected(const Fields& reply, std::string_view command)
{
    unsigned error = 0;
    return reply.size() == 3 && reply[0] == command && reply[1] == "ERROR" && number(reply[2], error);
}

GPSCommandOutcome readback(const Fields& reply, std::string_view command, bool matches)
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
