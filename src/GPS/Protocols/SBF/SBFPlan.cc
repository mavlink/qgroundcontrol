#include "SBF/SBFPlan.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "GPSCommandText.h"

namespace {

enum class When : uint8_t
{
    Always,
    /// COM ports have a baud rate; USB and IP connections do not.
    SerialPort,
    FixedBase,
    SurveyIn,
};

/// One receiver command, without its line terminator. {port}, {baud}, {latitude}, {longitude} and {altitude} are
/// filled in when the plan is built.
struct Step
{
    std::string_view command;
    /// Names the step in evidence instead of its command when the command carries base coordinates.
    std::string_view label{};
    When when = When::Always;
    bool required = true;
    unsigned attempts = 1;
};

constexpr std::chrono::milliseconds COMMAND_TIMEOUT{1000};

constexpr std::array SILENCE_CORRECTION_OUTPUT{
    Step{.command = "setDataInOut,COM1,,-RTCMv3-RTCMv2-CMRv2", .required = false},
    Step{.command = "setDataInOut,COM2,,-RTCMv3-RTCMv2-CMRv2", .required = false},
    Step{.command = "setDataInOut,USB1,,-RTCMv3-RTCMv2-CMRv2", .required = false},
    Step{.command = "setDataInOut,USB2,,-RTCMv3-RTCMv2-CMRv2", .required = false},
    Step{.command = "setDataInOut,USB3,,-RTCMv3-RTCMv2-CMRv2", .required = false},
    Step{.command = "setDataInOut,USB4,,-RTCMv3-RTCMv2-CMRv2", .required = false},
};

constexpr std::array PORT{
    Step{.command = "setSBFOutput, Stream1, {port}, none, off"},
    Step{.command = "setCOMSettings, {port}, baud{baud}", .when = When::SerialPort},
    Step{.command = "setDataInOut, {port}, Auto, SBF"},
    // WGS84 selects the global datum except when external corrections supply a datum.
    Step{.command = "setGeodeticDatum, WGS84"},
    // Applied again after the datum: navigation confirms its SBF stream, base mode the selected port.
    Step{.command = "setDataInOut, {port}, Auto, SBF", .attempts = 5},
};

constexpr std::array BASE{
    Step{.command = "setDataInOut, {port}, Auto, RTCMv3+SBF"},
    Step{.command = "setStaticPosGeodetic, Geodetic1, {latitude}, {longitude}, {altitude}, WGS84",
         .label = "setStaticPosGeodetic (fixed position)",
         .when = When::FixedBase},
    Step{.command = "setAntennaOffset, Main, 0.000000, 0.000000, 0.000000", .when = When::FixedBase},
    Step{.command = "setReceiverDynamics, Low, Static", .when = When::FixedBase},
    Step{.command = "setPVTMode, Static, , Geodetic1", .when = When::FixedBase},
    Step{.command = "setPVTMode, Static, All, auto", .when = When::SurveyIn},
    Step{.command = "setSBFOutput, Stream1, {port}, +PVTGeodetic, msec500"},
};

struct Context
{
    QByteArrayView port;
    const GPSBaseStationConfig::Fixed* fixed = nullptr;
};

bool applies(When when, const Context& context)
{
    switch (when) {
        case When::Always:
            return true;
        case When::SerialPort:
            return context.port.startsWith("COM");
        case When::FixedBase:
            return context.fixed;
        case When::SurveyIn:
            return !context.fixed;
    }
    return false;
}

QByteArray filled(std::string_view text, const Context& context)
{
    QByteArray command(text.data(), static_cast<qsizetype>(text.size()));
    command.replace("{port}", context.port);
    command.replace("{baud}", QByteArray::number(SBF::Plan::BAUD_RATE));
    if (context.fixed) {
        const auto& position = context.fixed->position;
        command.replace("{latitude}", gpsFixedDecimal(position.latitudeDegrees, 9));
        command.replace("{longitude}", gpsFixedDecimal(position.longitudeDegrees, 9));
        command.replace("{altitude}", gpsFixedDecimal(position.altitudeMeters, 4));
    }
    return command;
}

GPSCommandSequence sequence(std::span<const Step> steps, const Context& context)
{
    GPSCommandSequence result;
    for (const auto& step : steps) {
        if (!applies(step.when, context)) {
            continue;
        }
        const QByteArray command = filled(step.command, context);
        const QByteArray wire = command + '\n';
        result.steps.emplace_back(GPSCommandSequence::Command{
            .step = {.command = step.label.empty() ? wire.toStdString() : std::string(step.label),
                     .timeout = COMMAND_TIMEOUT,
                     .required = step.required},
            .wire = wire,
            .reply = GPSCommandSequence::RawReply{.accepted = "$R: " + command.toStdString(), .rejected = "$R?"},
            .attempts = step.attempts});
    }
    return result;
}

}  // namespace

namespace SBF::Plan {

GPSCommandSequence silenceCorrectionOutput()
{
    return sequence(SILENCE_CORRECTION_OUTPUT, {});
}

GPSCommandSequence port(QByteArrayView port)
{
    return sequence(PORT, {.port = port});
}

GPSCommandSequence base(QByteArrayView port, const GPSBaseStationConfig& base)
{
    return sequence(BASE, {.port = port, .fixed = std::get_if<GPSBaseStationConfig::Fixed>(&base.mode)});
}

}  // namespace SBF::Plan
