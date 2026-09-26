#include "Femto/FemtoPlan.h"

#include <string>
#include <utility>

#include "GPSCommandText.h"
#include "GPSEllipsoidPosition.h"

namespace Femto::Plan {

namespace {

QByteArray bytes(std::string_view text)
{
    return QByteArray(text.data(), static_cast<qsizetype>(text.size()));
}

GPSCommandSequence::Command command(const QByteArray& line, std::string_view accepted)
{
    return {.step = {line.toStdString(), RESPONSE_TIMEOUT},
            .wire = line,
            .reply = GPSCommandSequence::RawReply{std::string(accepted), std::string(REJECTED)}};
}

GPSCommandSequence::Command command(const Command& entry)
{
    return command(bytes(entry.line), entry.accepted);
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
    QByteArray line = bytes(FIX_POSITION);
    line.replace("{latitude}", gpsFixedDecimal(position.latitudeDegrees, 8))
        .replace("{longitude}", gpsFixedDecimal(position.longitudeDegrees, 8))
        .replace("{height}", gpsFixedDecimal(static_cast<double>(position.altitudeMeters), 5));
    auto fixed = command(line, FIX_POSITION_ACCEPTED);
    fixed.step.command = "FIX POSITION (fixed position)";
    return {{std::move(fixed), command(GGA_OUTPUT)}};
}

}  // namespace Femto::Plan
