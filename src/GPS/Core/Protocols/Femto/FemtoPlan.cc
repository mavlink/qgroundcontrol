#include "Femto/FemtoPlan.h"

#include <QtCore/QByteArrayView>

#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"

namespace Femto::Plan {

namespace {

GPSCommandSequence::RawReply reply(std::string_view accepted)
{
    return {QByteArrayView(accepted).toByteArray(), QByteArrayView(REJECTED).toByteArray()};
}

GPSCommandSequence::Command command(const Command& entry)
{
    return gpsCommand(QByteArrayView(entry.line).toByteArray(), RESPONSE_TIMEOUT, reply(entry.accepted));
}

}  // namespace

GPSCommandSequence sequence(std::span<const Command> commands)
{
    GPSCommandSequence result;
    for (const Command& entry : commands) {
        result.steps.emplace_back(command(entry));
    }
    return result;
}

GPSCommandSequence fixedBase(const GPSEllipsoidPosition& position)
{
    const QByteArray line = "FIX POSITION " + gpsFixedDecimal(position.latitudeDegrees, 8) + ' ' +
                            gpsFixedDecimal(position.longitudeDegrees, 8) + ' ' +
                            gpsFixedDecimal(position.altitudeMeters, 5) + "\r\n";
    return {
        {command(ELLIPSOID_HEIGHTS),
         gpsCommand(line, RESPONSE_TIMEOUT, reply(FIX_POSITION_ACCEPTED), {.label = "FIX POSITION (fixed position)"}),
         command(GGA_OUTPUT)}};
}

}  // namespace Femto::Plan
