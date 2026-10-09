#include "SBF/SBFPlan.h"

#include <chrono>
#include <utility>
#include <variant>

#include "GPSProtocolMath.h"

namespace {

constexpr std::chrono::milliseconds COMMAND_TIMEOUT{1000};

/// Appends @a command, which the receiver echoes as "$R: <command>" when it accepts it.
void add(GPSCommandSequence& sequence, const QByteArray& command, GPSCommandOptions options = {})
{
    sequence.steps.emplace_back(
        gpsCommand(command + '\n', COMMAND_TIMEOUT,
                   GPSCommandSequence::RawReply{.accepted = "$R: " + command, .rejected = "$R?"}, std::move(options)));
}

}  // namespace

namespace SBF::Plan {

GPSCommandSequence silenceCorrectionOutput()
{
    GPSCommandSequence result;
    for (const char* port : {"COM1", "COM2", "USB1", "USB2", "USB3", "USB4"}) {
        add(result, "setDataInOut," + QByteArray(port) + ",,-RTCMv3-RTCMv2-CMRv2", {.required = false});
    }
    return result;
}

GPSCommandSequence port(QByteArrayView port, unsigned baud)
{
    const QByteArray name = port.toByteArray();
    GPSCommandSequence result;
    add(result, "setSBFOutput, Stream1, " + name + ", none, off");
    // COM ports have a baud rate; USB and IP connections do not.
    if (port.startsWith("COM")) {
        add(result, "setCOMSettings, " + name + ", baud" + QByteArray::number(baud));
    }
    add(result, "setDataInOut, " + name + ", Auto, SBF");
    // WGS84 selects the global datum except when external corrections supply a datum.
    add(result, "setGeodeticDatum, WGS84");
    return result;
}

GPSCommandSequence base(QByteArrayView port, const GPSBaseStationConfig& base)
{
    const QByteArray name = port.toByteArray();
    GPSCommandSequence result;
    add(result, "setDataInOut, " + name + ", Auto, RTCMv3+SBF");
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&base.mode)) {
        const auto& position = fixed->position;
        add(result,
            "setStaticPosGeodetic, Geodetic1, " + gpsFixedDecimal(position.latitudeDegrees, 9) + ", " +
                gpsFixedDecimal(position.longitudeDegrees, 9) + ", " + gpsFixedDecimal(position.altitudeMeters, 4) +
                ", WGS84",
            {.label = "setStaticPosGeodetic (fixed position)"});
        add(result, "setAntennaOffset, Main, 0.000000, 0.000000, 0.000000");
        add(result, "setReceiverDynamics, Low, Static");
        add(result, "setPVTMode, Static, , Geodetic1");
    } else {
        add(result, "setPVTMode, Static, All, auto");
    }
    add(result, "setSBFOutput, Stream1, " + name + ", +PVTGeodetic, msec500");
    return result;
}

}  // namespace SBF::Plan
