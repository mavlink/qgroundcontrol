#include "Ashtech/AshtechPlan.h"

#include <cmath>

#include <QtCore/QByteArrayView>

#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"

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
    return QByteArrayView(command).toByteArray() + "\r\n";
}

QByteArray line(std::string_view command, char port)
{
    return line(command).replace("{port}", QByteArray(1, port));
}

QByteArray surveyStart(std::chrono::seconds duration)
{
    return "$PASHS,POS,AVG," + QByteArray::number(static_cast<unsigned>(duration.count())) + "\r\n";
}

QByteArray fixedPosition(const GPSEllipsoidPosition& position)
{
    const double latitude = position.latitudeDegrees;
    const double longitude = position.longitudeDegrees;
    return "$PASHS,POS," + gpsFixedDecimal(degreesMinutes(latitude), 8) + (latitude < 0.0 ? ",S," : ",N,") +
           gpsFixedDecimal(degreesMinutes(longitude), 8) + (longitude < 0.0 ? ",W," : ",E,") +
           gpsFixedDecimal(position.altitudeMeters, 5) + ",PC1\r\n";
}

GPSCommandSequence::Command fixedPositionCommand(const GPSEllipsoidPosition& position)
{
    return gpsCommand(fixedPosition(position), RESPONSE_TIMEOUT, acknowledgement,
                      {.label = "$PASHS,POS (fixed position)"});
}

GPSCommandSequence sequence(std::span<const Setting> settings, char port)
{
    GPSCommandSequence result;
    for (const Setting& setting : settings) {
        result.steps.emplace_back(
            gpsCommand(line(setting.command, port), RESPONSE_TIMEOUT, acknowledgement, {.required = setting.required}));
    }
    return result;
}

}  // namespace Ashtech::Plan
