#include "Unicore/UnicorePlan.h"

namespace Unicore::Plan {

namespace {

QList<Command> role(const Command& baseMode, Reply baseReadback)
{
    return {
        {"UNLOG"},       // all logs off on this port
        {"MODE ROVER"},  // even when reconnecting to a receiver left in base mode
        {"MODE", Reply::RoverMode},
        {"GPGGA 1"},     // position every second
        {"GPGST 1"},     // position accuracy
        {"GPGSV 1"},     // satellites in view
        {"GPGSA 1"},     // satellites used and DOP
        baseMode,
        {"MODE", baseReadback},
    };
}

QList<Command> output()
{
    return {
        {"BESTNAVXYZA 1"},  // base status every second
        {"RTCM1005 1"},     // station coordinates
        {"RTCM1033 10"},    // receiver and antenna descriptors every 10 s
        {"RTCM1074 1"},     // GPS MSM4
        {"RTCM1084 1"},     // GLONASS MSM4
        {"RTCM1094 1"},     // Galileo MSM4
        {"RTCM1124 1"},     // BeiDou MSM4
    };
}

}  // namespace

Command identify()
{
    return {"VERSIONA", Reply::Version};
}

BaseStation averagingBase(std::chrono::seconds maximum)
{
    // Distance 0 forces newly averaged coordinates; it is not an accuracy threshold.
    const QByteArray mode = "MODE BASE TIME " + QByteArray::number(static_cast<qint64>(maximum.count())) + " 0";
    return {.role = role({mode}, Reply::AveragingMode), .output = output()};
}

BaseStation fixedBase(const GPSProtocolMath::Ecef& position)
{
    const QByteArray mode = "MODE BASE " + gpsFixedDecimal(position.x, 4) + ' ' + gpsFixedDecimal(position.y, 4) + ' ' +
                            gpsFixedDecimal(position.z, 4);
    // Read back before subscribing, so queued periodic output cannot satisfy the query.
    QList<Command> commands{{"BESTNAVXYZA", Reply::FixedPosition}};
    commands += output();
    return {.role = role({.text = mode, .label = "MODE BASE (fixed position)"}, Reply::FixedMode), .output = commands};
}

}  // namespace Unicore::Plan
