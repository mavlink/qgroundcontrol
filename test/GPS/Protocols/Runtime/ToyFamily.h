#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSCommandChannel.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSReceiverFamily.h"

Q_DECLARE_LOGGING_CATEGORY(ToyFamilyLog)

/// Worked example of a runtime family for an invented line protocol.
///
/// Host to receiver: "TOY?" identity poll, "TOYRATE,<hz>", "TOYSAT,ON", "TOYSVIN,<seconds>" or
/// "TOYFIX,<lat>,<lon>,<alt>", "TOYRTCM,ON" and the fire-and-forget "TOYDIAG". Receiver to host:
/// "$TOYID,<model>,<firmware>", "$TOYACK,<command>" or "$TOYNAK,<command>", a raw "RTCM ON" or "RTCM OFF" for the RTCM
/// switch, "$TOYPOS,<fix>,<lat>,<lon>,<satellites>", "$TOYSVIN,<active>,<valid>,<seconds>",
/// "$TOYWARN" (asks for diagnostics) and RTCM3 frames.
namespace Toy {

using namespace std::chrono_literals;

inline constexpr std::chrono::milliseconds COMMAND_TIMEOUT{200};
inline constexpr unsigned BAUD_RATES[] = {9600, 115200};

class Protocol final : public GPSFamilyProtocol
{
public:
    explicit Protocol(const GPSFamilyOptions& options)
        : _satelliteInfo(options.satelliteInfoEnabled)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override
    {
        _ready = false;
        _identity.clear();
        // Corrections are only forwarded from a configured receiver.
        channel.stream().setEnabled(GPSFrameKind::RTCM3, false);
        if (!channel.validateConfiguration(config)) {
            co_return false;
        }
        const auto detection =
            co_await channel.detectBaud(BAUD_RATES, baud, [this, &channel](unsigned) -> GPSTask<GPSBaudProbe> {
                const auto identity = co_await channel.transact(
                    {"TOY?", COMMAND_TIMEOUT}, "TOY?\r\n",
                    GPSTextMatcher([this](std::string_view reply) { return _identityReply(reply); }));
                co_return identity.succeeded() ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
            });
        if (!detection.found) {
            _warnUnanswered();
            co_return false;
        }
        baud = detection.baud;

        GPSCommandSequence plan{{_command("TOYRATE,1"), _command("TOYSAT,ON", false)}};
        if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
            plan.steps.emplace_back(_command(QStringLiteral("TOYFIX,%1,%2,%3")
                                                 .arg(fixed->position.latitudeDegrees, 0, 'f', 7)
                                                 .arg(fixed->position.longitudeDegrees, 0, 'f', 7)
                                                 .arg(double(fixed->position.altitudeMeters), 0, 'f', 2)
                                                 .toLatin1()));
        } else {
            const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode);
            plan.steps.emplace_back(_command("TOYSVIN," + QByteArray::number(qint64(survey.duration.count()))));
        }
        plan.steps.emplace_back(
            GPSCommandSequence::Command{.step = {"TOYRTCM,ON", COMMAND_TIMEOUT},
                                        .wire = "TOYRTCM,ON\r\n",
                                        .reply = GPSCommandSequence::RawReply{"RTCM ON", "RTCM OFF"}});
        const auto result = co_await channel.runSequence(plan);
        if (!result.succeeded()) {
            _warnFailedStep(*result.failedStep);
            co_return false;
        }
        channel.stream().setEnabled(GPSFrameKind::RTCM3, true);
        if (std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode)) {
            // Published outside decoding, so it joins the batch the runtime flushes after configuration.
            channel.context().sink().publishSurvey(true, false, {});
        }
        _ready = true;
        co_return true;
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            context.sink().publishRTCM(frame.bytes);
            return {};
        }
        const std::string_view line = frame.text();
        if (line.starts_with("$TOYPOS,")) {
            return _decodePosition(line.substr(8), context);
        }
        if (line.starts_with("$TOYSVIN,")) {
            const auto fields = _fields<3>(line.substr(9));
            context.sink().publishSurvey(fields[0] == 1, fields[1] == 1, std::chrono::seconds(fields[2]));
            return GPSReceiveUpdate::Activity;
        }
        if (line == "$TOYWARN") {
            _diagnosticsPending = true;
            return GPSReceiveUpdate::Activity;
        }
        context.offerReply(line);
        return line.starts_with("$TOY") ? GPSReceiveUpdates(GPSReceiveUpdate::Activity) : GPSReceiveUpdates{};
    }

    /// A session failure ends readiness, as for Unicore and Quectel.
    bool receiverReady(const GPSDecodeContext& context) const override { return _ready && !context.failed(); }

    QString identity() const override { return _identity; }

    GPSTask<void> serviceStreaming(GPSCommandChannel& channel) override
    {
        if (_diagnosticsPending) {
            _diagnosticsPending = false;
            (void) co_await channel.writeCommand({"TOYDIAG", COMMAND_TIMEOUT}, "TOYDIAG\r\n");
        }
    }

private:
    // Logging stays outside the coroutine bodies: in a header, a logging macro inside a coroutine frame trips GCC's
    // -Wsubobject-linkage.
    static void _warnUnanswered() { qCWarning(ToyFamilyLog) << "No toy receiver answered"; }

    /// The step label may carry the base position, so only its index is logged.
    static void _warnFailedStep(size_t step) { qCWarning(ToyFamilyLog) << "Toy configuration failed at step" << step; }

    static GPSCommandSequence::Command _command(const QByteArray& text, bool required = true)
    {
        return {.step = {text.toStdString(), COMMAND_TIMEOUT, {}, required},
                .wire = text + "\r\n",
                .reply = GPSTextMatcher([text](std::string_view reply) {
                    const std::string_view command(text.constData(), static_cast<size_t>(text.size()));
                    if (reply.starts_with("$TOYACK,") && reply.substr(8) == command) {
                        return GPSCommandOutcome::Acknowledged;
                    }
                    return reply.starts_with("$TOYNAK,") && reply.substr(8) == command ? GPSCommandOutcome::Rejected
                                                                                       : GPSCommandOutcome::Pending;
                })};
    }

    GPSCommandOutcome _identityReply(std::string_view reply)
    {
        if (!reply.starts_with("$TOYID,")) {
            return GPSCommandOutcome::Pending;
        }
        const QString fields = QString::fromLatin1(reply.substr(7));
        _identity = fields.section(QLatin1Char(','), 0, 0) + QLatin1Char(' ') + fields.section(QLatin1Char(','), 1);
        return GPSCommandOutcome::ReadbackVerified;
    }

    template <size_t N>
    static std::array<int, N> _fields(std::string_view text)
    {
        std::array<int, N> values{};
        for (auto& value : values) {
            const auto comma = text.find(',');
            (void) std::from_chars(text.data(), text.data() + std::min(comma, text.size()), value);
            text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        }
        return values;
    }

    GPSReceiveUpdates _decodePosition(std::string_view body, GPSDecodeContext& context)
    {
        const QStringList fields = QString::fromLatin1(body).split(QLatin1Char(','));
        if (fields.size() != 4) {
            return {};
        }
        GPSDecodedPosition position;
        position.navigation.timestampUs = context.nowUs();
        position.navigation.fixType = static_cast<GPSFixQuality>(fields[0].toInt());
        position.navigation.latitudeDegrees = fields[1].toDouble();
        position.navigation.longitudeDegrees = fields[2].toDouble();
        position.navigation.satellitesUsed = static_cast<uint8_t>(fields[3].toUInt());
        if (_satelliteInfo) {
            context.sink().publishSatelliteUsage(fields[3].toInt());
        }
        context.sink().publishPosition(position);
        return GPSReceiveUpdate::Activity;
    }

    QString _identity;
    bool _satelliteInfo = true;
    bool _ready = false;
    bool _diagnosticsPending = false;
};

inline std::unique_ptr<GPSFamilyProtocol> create(const GPSFamilyOptions& options)
{
    return std::make_unique<Protocol>(options);
}

inline constexpr GPSReceiverFamily FAMILY{
    .type = GPSType::passive,
    .name = QLatin1StringView("Toy"),
    .logCategory = &ToyFamilyLog,
    .stream = {.framers = GPSFrameKind::ASCIILine | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::ASCIILine},
    .baudCandidates = BAUD_RATES,
    .create = &create,
};

}  // namespace Toy
