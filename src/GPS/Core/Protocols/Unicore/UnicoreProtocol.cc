#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GPSASCIILog.h"
#include "GPSCommandChannel.h"
#include "GPSNMEAFamilyProtocol.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverFamilies.h"
#include "GPSStreamDemux.h"
#include "GPSTime.h"
#include "MonotonicClock.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "Unicore/UnicorePlan.h"

QGC_LOGGING_CATEGORY(UnicoreProtocolLog, "GPS.Protocols.Unicore")

// Independent implementation of Unicore N4 Commands and Logs Reference Book, EN R1.6:
// https://en.unicore.com/uploads/file/Unicore%20Reference%20Commands%20Manual%20For%20N4%20High%20Precision%20Products_V2_EN_R1.6.pdf
// Sections 3, 7.3.1, 7.3.27, 7.3.44; Appendices 1/2 and position/solution status tables.
// Captured command-reply framing (XOR includes '$'):
// https://s-taka.org/control-command-for-gnss-receiver-um982/
namespace {

namespace Plan = Unicore::Plan;

std::string_view unquote(std::string_view field)
{
    if (field.size() < 2 || field.front() != '"' || field.back() != '"') {
        return {};
    }
    field.remove_prefix(1);
    field.remove_suffix(1);
    return field.find('"') == std::string_view::npos ? field : std::string_view{};
}

bool supportedFirmware(std::string_view model, std::string_view firmware)
{
    constexpr std::string_view PREFIX = "R4.10Build";
    if (!firmware.starts_with(PREFIX)) {
        return false;
    }
    const auto build = NMEA::number<unsigned>(firmware.substr(PREFIX.size()));
    // N4 R1.6 section 3.6 explicitly gives these minimum builds for the rover modes.
    return build && ((model == "UM980" && *build >= 7923) || (model == "UM982" && *build >= 7650));
}

/// Whether @a line ends in the XOR checksum of everything before its '*', including the leading '$' or '#', as
/// command replies and MODE logs carry it, unlike NMEA.
bool validXorChecksum(std::string_view line)
{
    const auto star = line.rfind('*');
    return star != std::string_view::npos && star + 3 == line.size() &&
           NMEA::hexNumber(line.substr(star + 1)) == NMEA::checksum(line.substr(0, star));
}

/// Whether the MODE log's @a mode is the role @a reply reads back.
bool modeMatches(std::string_view mode, Plan::Reply reply)
{
    const auto starts = [mode](std::string_view name) {
        return mode == name || (mode.starts_with(name) && mode.size() > name.size() && mode[name.size()] == ' ');
    };
    switch (reply) {
        case Plan::Reply::RoverMode:
            return starts("MODE ROVER");
        case Plan::Reply::AveragingMode:
            return starts("MODE BASE TIME");
        default:
            return mode == "MODE BASE";
    }
}

bool samePosition(const GPSProtocolMath::Ecef& left, const GPSProtocolMath::Ecef& right)
{
    // ECEF command/readback values have four decimal places; this is not survey accuracy.
    return GPSProtocolMath::distance(left, right) < 0.02;
}

/// Configures a UM980/UM982 as an RTK base: finds it at one of Plan::BAUD_RATES, or at the rate configure() gets when
/// nonzero, which then reports the rate found, and runs Plan::averagingBase() or Plan::fixedBase(). Each command's
/// reply matcher verifies its acknowledgement or readback. Receiver-managed averaging is the only survey mode, and
/// persistent changes are not supported.
///
/// Decodes standard NMEA epochs, RTCM3, command acknowledgements and the VERSIONA, MODE and BESTNAVXYZA logs, offering
/// each valid reply to the pending command, and monitors the base: the base is valid while fresh BESTNAVXYZA epochs
/// report FIXEDPOS at the configured coordinates, at most STATUS_MAX_AGE apart. Once ready, a reboot, another mode
/// or a lost base ends readiness.
class Protocol final : public GPSNMEAFamilyProtocol
{
public:
    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    /// Model and firmware from VERSIONA.
    QString identity() const override { return gpsReceiverIdentity(_model, _firmware); }

    /// The state a successful configuration for a fixed or averaging base leaves, without publishing an initial status.
    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext&) override
    {
        if (std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode)) {
            return false;
        }
        _setBase(config.base.mode);
        _monitorBase = true;
        _ready = true;
        return true;
    }

    std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const override
    {
        return (std::min) (timeout, STATUS_POLL);
    }

    /// The identity query configure() probes each rate with. A receiver that reports its model answered, even when its
    /// firmware is not supported.
    bool probe(GPSCommandChannel& channel) override { return _identify(channel).succeeded() || !_model.isEmpty(); }

private:
    /// Starts a configuration attempt: forgets the receiver and its stream, stops corrections and revokes the base.
    void _startAttempt(GPSStreamDemux& stream, GPSDecodeContext& context);
    /// The base the receiver must run: its mode readback and, for a fixed base, its coordinates.
    void _setBase(const GPSBaseStationConfig::Mode& mode);
    /// Sends Plan::identify(). Clears _rejection.
    GPSCommandSequence::Result _identify(GPSCommandChannel& channel);
    /// Commands whose replies _match() verifies.
    GPSCommandSequence _sequence(const QList<Plan::Command>& commands);
    GPSCommandOutcome _match(const Plan::Command& command, std::string_view line);
    /// Reports base status from BESTNAVXYZA from now on; an averaging base is active until it reports FIXEDPOS.
    void _monitorBaseStatus(GPSDecodeContext& context);
    /// The configuration failed: revokes a monitored base and stops corrections.
    bool _failAttempt(GPSCommandChannel& channel);
    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) override;
    /// Each handler returns whether its log was valid.
    bool _handleVersion(std::string_view body, GPSDecodeContext& context);
    bool _handleMode(std::string_view body, GPSDecodeContext& context);
    bool _handlePosition(std::string_view body, GPSDecodeContext& context);
    void _publishBase(GPSDecodeContext& context, bool active, bool valid);
    /// Ends readiness; @a reason becomes the error detail unless a failure is already recorded.
    void _invalidateBase(GPSDecodeContext& context, const QString& reason);
    /// Ends readiness when base status expired or a failure is recorded.
    void _expire(GPSDecodeContext& context) override;

    /// The role MODE must report once ready.
    Plan::Reply _expectedMode = Plan::Reply::AveragingMode;
    QByteArray _model;
    QByteArray _firmware;
    /// Why the receiver that answered the identity query is not supported; empty otherwise.
    QString _rejection;
    /// The latest valid MODE and BESTNAVXYZA reports.
    QByteArray _reportedMode;
    GPSProtocolMath::Ecef _reportedPosition;
    bool _reportedFixed = false;
    /// The pending command's acknowledgement was offered.
    bool _acknowledgementMatched = false;
    /// The coordinates a fixed base must report.
    GPSProtocolMath::Ecef _fixedECEF;
    GPSProtocolMath::Ecef _baseECEF;
    uint64_t _lastBaseStatus = 0;
    std::optional<uint64_t> _lastBaseEpoch;
    bool _monitorBase = false;
    bool _baseValid = false;
    bool _averaging = false;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _startAttempt(channel.stream(), channel.context());
    (void) channel.flush();
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode);
    const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&config.base.mode);
    if (!fixed && !averaging) {
        channel.failControl(
            QStringLiteral("Unicore requires receiver-managed averaging with a duration between 1 and 3600 seconds"));
        return _failAttempt(channel);
    }
    _setBase(config.base.mode);
    const auto scope = channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    GPSCommandSequence::Result identity;
    const auto detection = channel.detectBaud(Plan::BAUD_RATES, baud, [this, &channel, &identity]() -> GPSBaudProbe {
        _nmea.reset(channel.stream());
        identity = _identify(channel);
        if (identity.succeeded()) {
            return GPSBaudProbe::Found;
        }
        // A receiver that reported its model answered at this rate.
        return _model.isEmpty() ? GPSBaudProbe::TryNext : GPSBaudProbe::Stop;
    });
    if (!detection.found) {
        if (!_rejection.isEmpty()) {
            channel.failControl(_rejection);
        } else if (identity.outcome == GPSCommandOutcome::Rejected) {
            channel.failSequence(identity);
        } else {
            channel.failNoAnswer(detection.probed);
        }
        return _failAttempt(channel);
    }
    baud = detection.baud;
    qCDebug(UnicoreProtocolLog).noquote() << "Unicore" << _model << "firmware" << _firmware;
    const auto plan = fixed ? Plan::fixedBase(_fixedECEF) : Plan::averagingBase(averaging->maximumDuration);
    if (!channel.runRequired(_sequence(plan.role))) {
        return _failAttempt(channel);
    }
    _monitorBaseStatus(channel.context());
    if (!channel.runRequired(_sequence(plan.output))) {
        return _failAttempt(channel);
    }
    // Corrections flow while the base is valid.
    _ready = true;
    _nmea.setRTCMEnabled(_baseValid);
    return true;
}

void Protocol::_startAttempt(GPSStreamDemux& stream, GPSDecodeContext& context)
{
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _lastBaseEpoch.reset();
    _nmea.setRTCMEnabled(false);
    _nmea.reset(stream);
    _model.clear();
    _firmware.clear();
    _rejection.clear();
    _publishBase(context, false, false);
}

void Protocol::_setBase(const GPSBaseStationConfig::Mode& mode)
{
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&mode);
    _averaging = !fixed;
    _expectedMode = fixed ? Plan::Reply::FixedMode : Plan::Reply::AveragingMode;
    if (fixed) {
        _fixedECEF = GPSProtocolMath::toEcef(fixed->position);
    }
}

GPSCommandSequence::Result Protocol::_identify(GPSCommandChannel& channel)
{
    _rejection.clear();
    return channel.runSequence(_sequence({Plan::identify()}));
}

GPSCommandSequence Protocol::_sequence(const QList<Plan::Command>& commands)
{
    GPSCommandSequence result;
    for (const Plan::Command& command : commands) {
        result.steps.push_back(gpsCommand(command.text + "\r\n", Plan::COMMAND_TIMEOUT,
                                          [this, command](std::string_view line) { return _match(command, line); },
                                          {.label = command.label}));
    }
    return result;
}

GPSCommandOutcome Protocol::_match(const Plan::Command& command, std::string_view line)
{
    if (line.starts_with("$command,")) {
        const auto response = line.find(",response: ", 9);
        if (response == std::string_view::npos ||
            QByteArrayView(line.substr(9, response - 9)).compare(command.text, Qt::CaseInsensitive) != 0) {
            return GPSCommandOutcome::Pending;
        }
        _acknowledgementMatched = true;
        if (line.substr(response + 11, line.rfind('*') - response - 11) != "OK") {
            return GPSCommandOutcome::Rejected;
        }
        return command.reply == Plan::Reply::Acknowledgement ? GPSCommandOutcome::Acknowledged
                                                             : GPSCommandOutcome::Pending;
    }
    switch (command.reply) {
        case Plan::Reply::Acknowledgement:
            return GPSCommandOutcome::Pending;
        case Plan::Reply::Version:
            if (!line.starts_with("#VERSIONA,")) {
                return GPSCommandOutcome::Pending;
            }
            if (supportedFirmware(_model.toStdString(), _firmware.toStdString())) {
                return GPSCommandOutcome::ReadbackVerified;
            }
            _rejection = QStringLiteral(
                             "Unsupported Unicore receiver '%1' firmware '%2'; requires UM980 R4.10Build7923+ "
                             "or UM982 R4.10Build7650+")
                             .arg(QString::fromUtf8(_model), QString::fromUtf8(_firmware));
            return GPSCommandOutcome::Rejected;
        case Plan::Reply::RoverMode:
        case Plan::Reply::AveragingMode:
        case Plan::Reply::FixedMode:
            if (!line.starts_with("#MODE,")) {
                return GPSCommandOutcome::Pending;
            }
            return modeMatches(_reportedMode.toStdString(), command.reply) ? GPSCommandOutcome::ReadbackVerified
                                                                           : GPSCommandOutcome::Rejected;
        case Plan::Reply::FixedPosition:
            if (!line.starts_with("#BESTNAVXYZA,") || !_reportedFixed) {
                return GPSCommandOutcome::Pending;
            }
            return samePosition(_reportedPosition, _fixedECEF) ? GPSCommandOutcome::ReadbackVerified
                                                               : GPSCommandOutcome::Rejected;
    }
    return GPSCommandOutcome::Pending;
}

void Protocol::_monitorBaseStatus(GPSDecodeContext& context)
{
    _monitorBase = true;
    _publishBase(context, _averaging, false);
}

bool Protocol::_failAttempt(GPSCommandChannel& channel)
{
    if (_monitorBase) {
        _publishBase(channel.context(), false, false);
    }
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _nmea.setRTCMEnabled(false);
    (void) channel.flush();
    return false;
}

GPSReceiveUpdates Protocol::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    if (line.starts_with("$command,")) {
        if (!context.replyPending() || !validXorChecksum(line)) {
            return {};
        }
        _acknowledgementMatched = false;
        context.offerReply(line);
        return _acknowledgementMatched ? GPSReceiveUpdates(GPSReceiveUpdate::Activity) : GPSReceiveUpdates{};
    }
    if (!line.starts_with('#')) {
        return {};
    }
    const auto comma = line.find(',');
    const auto name = line.substr(1, comma - 1);
    if (name != "VERSIONA" && name != "MODE" && name != "BESTNAVXYZA") {
        return {};
    }
    // MODE carries an XOR checksum; the other logs a CRC-32.
    if (name == "MODE" ? !validXorChecksum(line) : !gpsASCIILog(line)) {
        return {};
    }
    const auto star = line.rfind('*');
    const auto semicolon = line.find(';');
    if (semicolon == std::string_view::npos || semicolon >= star) {
        return {};
    }
    std::array<std::string_view, 10> header{};
    if (NMEA::splitFields(line.substr(1, semicolon - 1), header) != header.size()) {
        return {};
    }
    const auto body = line.substr(semicolon + 1, star - semicolon - 1);
    bool valid = false;
    if (name == "VERSIONA") {
        valid = _handleVersion(body, context);
    } else if (name == "MODE") {
        valid = _handleMode(body, context);
    } else {
        if (!_monitorBase) {
            return GPSReceiveUpdate::Activity;
        }
        const auto week = NMEA::number<uint16_t>(header[4]);
        const auto milliseconds = NMEA::number<uint32_t>(header[5]);
        if (header[2] != "GPS" || header[3] != "FINE" || !week || !milliseconds || *milliseconds >= GPSTime::WEEK_MS) {
            return {};
        }
        const uint64_t epoch = GPSTime::epochMs(*week, *milliseconds);
        if (_lastBaseEpoch && epoch < *_lastBaseEpoch) {
            _invalidateBase(context, QStringLiteral("Unicore base status went back in time; the receiver may have "
                                                    "restarted"));
            return GPSReceiveUpdate::Activity;
        }
        if (_lastBaseEpoch && epoch == *_lastBaseEpoch) {
            return GPSReceiveUpdate::Activity;
        }
        _lastBaseEpoch = epoch;
        valid = _handlePosition(body, context);
    }
    if (valid) {
        context.offerReply(line);
    }
    return GPSReceiveUpdate::Activity;
}

bool Protocol::_handleVersion(std::string_view body, GPSDecodeContext& context)
{
    std::array<std::string_view, 6> fields{};
    if (NMEA::splitFields(body, fields) != fields.size()) {
        return false;
    }
    const auto model = unquote(fields[0]);
    const auto firmware = unquote(fields[1]);
    if (model.empty() || model.size() > 32 || firmware.empty() || firmware.size() > 32) {
        return false;
    }
    if (context.replyPending()) {
        _model = QByteArrayView(model).toByteArray();
        _firmware = QByteArrayView(firmware).toByteArray();
    } else if (_ready) {
        // An unsolicited identity report can indicate a reboot; never retain old base validity.
        _invalidateBase(context,
                        QStringLiteral("Unicore receiver reported its identity unasked; it may have restarted"));
    }
    return true;
}

bool Protocol::_handleMode(std::string_view body, GPSDecodeContext& context)
{
    const auto mode = body.substr(0, body.find(','));
    _reportedMode = QByteArrayView(mode).toByteArray();
    if (_ready && !modeMatches(mode, _expectedMode)) {
        _invalidateBase(context, QStringLiteral("Unicore receiver left the configured base mode"));
    }
    return true;
}

bool Protocol::_handlePosition(std::string_view body, GPSDecodeContext& context)
{
    std::array<std::string_view, 28> fields{};
    if (NMEA::splitFields(body, fields) != fields.size()) {
        return false;
    }
    const auto x = NMEA::number<double>(fields[2]);
    const auto y = NMEA::number<double>(fields[3]);
    const auto z = NMEA::number<double>(fields[4]);
    if (!x || !y || !z) {
        return false;
    }
    const GPSProtocolMath::Ecef coordinates{*x, *y, *z};
    const bool fixed =
        fields[0] == "SOL_COMPUTED" && fields[1] == "FIXEDPOS" && GPSProtocolMath::nearEarthSurface(coordinates);
    _reportedPosition = coordinates;
    _reportedFixed = fixed;
    const bool matches = _averaging || samePosition(coordinates, _fixedECEF);
    if (_ready && _baseValid && (!fixed || !matches || !samePosition(coordinates, _baseECEF))) {
        _invalidateBase(context, QStringLiteral("Unicore base position was lost or changed"));
        return true;
    }
    _lastBaseStatus = context.nowUs();
    _baseValid = fixed && matches;
    if (_baseValid) {
        _baseECEF = coordinates;
    }
    _nmea.setRTCMEnabled(_ready && _baseValid);
    _publishBase(context, _averaging && !_baseValid, _baseValid);
    return true;
}

void Protocol::_publishBase(GPSDecodeContext& context, bool active, bool valid)
{
    // BESTNAV's instantaneous sigmas and BASEPOS monitoring are not averaging accuracy or elapsed time.
    context.publishSurvey(active, valid, {}, valid ? GPSProtocolMath::fromEcef(_baseECEF) : GPSEllipsoidPosition{});
}

void Protocol::_invalidateBase(GPSDecodeContext& context, const QString& reason)
{
    _baseValid = false;
    _monitorBase = false;
    _ready = false;
    _nmea.setRTCMEnabled(false);
    _publishBase(context, false, false);
    context.failControl(reason);
}

void Protocol::_expire(GPSDecodeContext& context)
{
    if (_ready && (context.failed() ||
                   (_baseValid && !MonotonicClock::withinAge(_lastBaseStatus, context.nowUs(), STATUS_MAX_AGE)))) {
        _invalidateBase(context, QStringLiteral("Unicore base status stopped arriving"));
    }
}

/// Unicore logs carry the CPU idle time where NovAtel-compatible receivers name the port.
QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    if (text.starts_with("$command,") && text.find(",response: ") != std::string_view::npos && validXorChecksum(text)) {
        return QLatin1StringView("Unicore command replies");
    }
    const auto log = gpsASCIILog(text);
    const bool unicore = log && !log->source.empty() && std::ranges::all_of(log->source, NMEA::isAsciiDigit);
    return unicore ? QLatin1StringView("Unicore ASCII logs") : QLatin1StringView();
}

}  // namespace

namespace Unicore {

const GPSReceiverFamily FAMILY{
    .type = GPSType::unicore,
    .logCategory = &UnicoreProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace Unicore
