#include "Ashtech/AshtechPlan.h"

#include <cmath>

#include "GPSCommandText.h"
#include "GPSEllipsoidPosition.h"

namespace Ashtech::Plan {

namespace {

/// Unsigned ddmm.mmmmmm.
double degreesMinutes(double degrees)
{
    const double magnitude = std::abs(degrees);
    const double whole = std::trunc(magnitude);
    return whole * 100.0 + (magnitude - whole) * 60.0;
}

}  // namespace

GPSCommandOutcome rejection(std::string_view reply)
{
    return reply.starts_with("$PASHR,NAK*") ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Pending;
}

GPSCommandOutcome acknowledgement(std::string_view reply)
{
    return reply.starts_with("$PASHR,ACK*") ? GPSCommandOutcome::Acknowledged : rejection(reply);
}

QByteArray line(std::string_view command)
{
    return QByteArray(command.data(), static_cast<qsizetype>(command.size())) + "\r\n";
}

QByteArray line(std::string_view command, char port)
{
    return line(command).replace("{port}", QByteArray(1, port));
}

QByteArray surveyStart(std::chrono::seconds duration)
{
    return line(SURVEY_START).replace("{seconds}", QByteArray::number(static_cast<unsigned>(duration.count())));
}

QByteArray fixedPosition(const GPSEllipsoidPosition& position)
{
    const double latitude = position.latitudeDegrees;
    const double longitude = position.longitudeDegrees;
    QByteArray text = line(FIXED_POSITION);
    text.replace("{latitude}", gpsFixedDecimal(degreesMinutes(latitude), 8))
        .replace("{N|S}", latitude < 0.0 ? "S" : "N")
        .replace("{longitude}", gpsFixedDecimal(degreesMinutes(longitude), 8))
        .replace("{E|W}", longitude < 0.0 ? "W" : "E")
        .replace("{height}", gpsFixedDecimal(static_cast<double>(position.altitudeMeters), 5));
    return text;
}

GPSCommandSequence::Command command(const QByteArray& line, GPSTextMatcher reply, bool required)
{
    return {.step = {line.toStdString(), RESPONSE_TIMEOUT, {}, required}, .wire = line, .reply = std::move(reply)};
}

GPSCommandSequence::Command fixedPositionCommand(const GPSEllipsoidPosition& position)
{
    auto result = command(fixedPosition(position));
    result.step.command = "$PASHS,POS (fixed position)";
    return result;
}

GPSCommandSequence sequence(std::span<const Setting> settings, char port)
{
    GPSCommandSequence result;
    for (const Setting& setting : settings) {
        result.steps.emplace_back(command(line(setting.command, port), acknowledgement, setting.required));
    }
    return result;
}

}  // namespace Ashtech::Plan
