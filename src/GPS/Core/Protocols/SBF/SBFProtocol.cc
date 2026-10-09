#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string_view>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolEvent.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverFamilies.h"
#include "GPSStreamDemux.h"
#include "GPSSurveyClock.h"
#include "GPSTime.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "SBF/SBFBlocks.h"
#include "SBF/SBFPlan.h"
#include "WireFields.h"

QGC_LOGGING_CATEGORY(SBFProtocolLog, "GPS.Protocols.SBF")

namespace {

constexpr double DNU = 100000.0;
constexpr qsizetype DESCRIPTOR_SIZE = 4;

GPSPositionReport::FixType fixType(uint8_t modeType)
{
    switch (modeType) {
        case 0:
            return GPSPositionReport::FixType::NoFix;
        case 2:
        case 6:
            return GPSPositionReport::FixType::Differential;
        case 5:
        case 8:
            return GPSPositionReport::FixType::RTKFloat;
        case 4:
        case 7:
            return GPSPositionReport::FixType::RTKFixed;
        default:
            return GPSPositionReport::FixType::Fix3D;
    }
}

/// The connection descriptor before a prompt's '>', such as "USB1", "COM2" or "IP10", at the start of @a text or after
/// a separator. Command replies before the prompt and line noise at a wrong rate are not taken for one.
std::optional<QByteArray> promptDescriptor(QByteArrayView text)
{
    for (qsizetype end = text.indexOf('>'); end >= 0; end = text.indexOf('>', end + 1)) {
        if (end < DESCRIPTOR_SIZE) {
            continue;
        }
        const QByteArrayView descriptor = text.sliced(end - DESCRIPTOR_SIZE, DESCRIPTOR_SIZE);
        const char before = end > DESCRIPTOR_SIZE ? text[end - DESCRIPTOR_SIZE - 1] : ' ';
        const bool separated = !(NMEA::isAsciiUpper(before) || NMEA::isAsciiDigit(before));
        const bool tail = (NMEA::isAsciiUpper(descriptor[2]) || NMEA::isAsciiDigit(descriptor[2])) &&
                          NMEA::isAsciiDigit(descriptor[3]);
        if (separated && NMEA::isAsciiUpper(descriptor[0]) && NMEA::isAsciiUpper(descriptor[1]) && tail) {
            return descriptor.toByteArray();
        }
    }
    return std::nullopt;
}

/// Asks for the command prompt and reads the connection descriptor it names, such as "USB1" in "USB1>", into @a port.
/// @return false when no prompt arrived within Plan::PROMPT_TIMEOUT.
bool detectPort(GPSCommandChannel& channel, QByteArray& port)
{
    const GPSDeadline deadline = GPSDeadline::after(channel.nowUs(), SBF::Plan::PROMPT_TIMEOUT);
    if (!channel.write(SBF::Plan::PROMPT)) {
        return false;
    }
    QByteArray received;
    std::array<uint8_t, GPSCommandChannel::READ_CHUNK_SIZE> chunk{};
    while (channel.nowUs() < deadline.untilUs) {
        const int count = channel.read(chunk, deadline.remaining(channel.nowUs()));
        if (count < 0) {
            return false;
        }
        received.append(reinterpret_cast<const char*>(chunk.data()), count);
        if (const auto descriptor = promptDescriptor(received)) {
            port = *descriptor;
            qCDebug(SBFProtocolLog).noquote() << "Septentrio GNSS receiver COM port:" << port;
            return true;
        }
        // Keep enough for a prompt split across reads.
        if (received.size() > 2 * static_cast<qsizetype>(chunk.size())) {
            received = received.last(DESCRIPTOR_SIZE + 1);
        }
    }
    return false;
}

/// Configures a Septentrio receiver as an RTK base over the connection QGC reaches it through. The link runs at the
/// rate selected or detected, else Plan::BAUD_RATE; a serial port of the receiver is set to the same rate. With
/// command input forced and correction output silenced, the command prompt names the connection, which the port and
/// base plans then address; RTCM3 framing starts once that port is set to stream it. A failed command leaves a
/// description in the channel's error detail. Decodes SBF block candidates and RTCM3 frames: base stations output only
/// PVTGeodetic, which carries both the navigation solution and the survey-in status, so each block is one position
/// epoch.
class Protocol final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    bool probe(GPSCommandChannel& channel) override;

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override;

    /// Whether the receiver is configured as a base. A position datum other than WGS84 ends it.
    bool receiverReady() const override { return _ready; }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override
    {
        context.stream().setEnabled(GPSFrameKind::RTCM3, true);
        _startBase(config.base, context.nowUs());
        return true;
    }

    GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        if (!_ready || channel.failed()) {
            return GPSReceiveUpdates{};
        }
        return channel.receiveCycle(timeout);
    }

private:
    bool _configureBase(GPSCommandChannel& channel, const GPSBaseStationConfig& base, unsigned baud);
    /// Starts reporting survey status for @a base; a survey-in is timed from @a nowUs.
    void _startBase(const GPSBaseStationConfig& base, uint64_t nowUs);
    GPSReceiveUpdates _decodeBlock(const GPSFrame& frame, GPSDecodeContext& context);
    /// Fills @a position from one PVTGeodetic block. @return whether its coordinates are usable.
    bool _applyPVTGeodetic(const SBF::PVTGeodetic& pvt, GPSDecodedPosition& position, GPSDecodeContext& context) const;
    void _publishSurveyStatus(const SBF::PVTGeodetic& pvt, const GPSDecodedPosition& position, bool coordinatesValid,
                              GPSDecodeContext& context);

    bool _ready = false;
    GPSBaseStationConfig _base;
    GPSSurveyClock _surveyClock;
    bool _surveyActive = false;
    /// Receiver time of the last published epoch, in milliseconds since the GPS epoch.
    std::optional<uint64_t> _lastEpoch;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    // A new configuration stops reporting the base, forgets survey progress and accepts any receiver time.
    _ready = false;
    _surveyClock.reset();
    _surveyActive = false;
    _lastEpoch.reset();
    channel.stream().setEnabled(GPSFrameKind::RTCM3, false);
    channel.stream().reset(GPSFrameKind::SBF);
    // The link keeps the rate selected or detected; otherwise it runs at Plan::BAUD_RATE.
    baud = config.linkBaud(baud, SBF::Plan::BAUD_RATE);
    if (!_configureBase(channel, config.base, baud)) {
        return false;
    }
    _startBase(config.base, channel.nowUs());
    return true;
}

bool Protocol::_configureBase(GPSCommandChannel& channel, const GPSBaseStationConfig& base, unsigned baud)
{
    (void) channel.setBaudrate(baud);
    (void) channel.write(SBF::Plan::FORCE_COMMAND_INPUT);

    if (!channel.runSequence(SBF::Plan::silenceCorrectionOutput()).succeeded()) {
        return false;
    }
    QByteArray port;
    if (!detectPort(channel, port)) {
        channel.failControl(QStringLiteral("No command prompt from the Septentrio receiver"));
        return false;
    }
    if (!channel.runRequired(SBF::Plan::port(port, baud))) {
        return false;
    }
    channel.stream().setEnabled(GPSFrameKind::RTCM3, true);
    return channel.runRequired(SBF::Plan::base(port, base)) && !channel.failed();
}

void Protocol::_startBase(const GPSBaseStationConfig& base, uint64_t nowUs)
{
    _base = base;
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_base.mode)) {
        _surveyClock.start(nowUs);
    }
    _ready = true;
}

GPSReceiveUpdates Protocol::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        context.publishRTCM(frame.bytes);
        return {};
    }
    return _decodeBlock(frame, context);
}

GPSReceiveUpdates Protocol::_decodeBlock(const GPSFrame& frame, GPSDecodeContext& context)
{
    const auto header = SBF::checkedHeader(frame.bytes);
    if (!header || header->number() != SBF::BlockId::PVT_GEODETIC || header->length < Wire::SIZE<SBF::PVTGeodetic>) {
        return {};
    }
    const auto pvt = Wire::decode<SBF::PVTGeodetic>(frame.bytes);
    if (pvt.tow >= GPSTime::WEEK_MS || pvt.wnc == UINT16_MAX) {
        return {};
    }
    const uint64_t epoch = GPSTime::epochMs(pvt.wnc, pvt.tow);
    if (_lastEpoch && epoch <= *_lastEpoch) {
        return GPSReceiveUpdate::Activity;
    }
    _lastEpoch = epoch;

    GPSDecodedPosition position;
    // A base reports in the WGS84/ITRS datum (0) it was configured for. Other receivers report in theirs, such as a
    // rover in its correction provider's datum (19), and are decoded as reported.
    if (_ready && pvt.datum != 0) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
        position.navigation.timestampUs = context.nowUs();
        qCWarning(SBFProtocolLog) << "Unsupported Septentrio position datum:" << unsigned(pvt.datum);
        context.failControl(QStringLiteral("Septentrio position datum is not WGS84/ITRS"));
        _ready = false;
        context.stream().setEnabled(GPSFrameKind::RTCM3, false);
        context.publishSurvey(false, false, {});
    } else {
        const bool coordinatesValid = _applyPVTGeodetic(pvt, position, context);
        if (_ready) {
            _publishSurveyStatus(pvt, position, coordinatesValid, context);
        }
    }
    context.publishPosition(position);
    return GPSReceiveUpdate::Activity;
}

bool Protocol::_applyPVTGeodetic(const SBF::PVTGeodetic& pvt, GPSDecodedPosition& position,
                                 GPSDecodeContext& context) const
{
    position.navigation.fixType = fixType(pvt.modeType());
    if (pvt.error != 0) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    } else if (pvt.mode2D() && position.navigation.fixType >= GPSPositionReport::FixType::Fix3D) {
        position.navigation.fixType = GPSPositionReport::FixType::Fix2D;
    }

    // Any value beyond the specified maximum is invalid, not only the do-not-use value (-2*10^10).
    position.velocityValid = position.navigation.fixType > GPSPositionReport::FixType::NoFix && pvt.error == 0 &&
                             !(fabsf(pvt.vn) > 600.0f || fabsf(pvt.ve) > 600.0f || fabsf(pvt.vu) > 600.0f);

    const bool coordinatesValid = std::isfinite(pvt.latitude) && std::abs(pvt.latitude) <= std::numbers::pi / 2 &&
                                  std::isfinite(pvt.longitude) && std::abs(pvt.longitude) <= std::numbers::pi &&
                                  std::isfinite(pvt.height) && std::abs(pvt.height) <= DNU;
    if (!coordinatesValid || !std::isfinite(pvt.undulation) || std::abs(pvt.undulation) > DNU) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    }

    const auto satellitesUsed = gpsSatellitesUsed(pvt.nrSV);  // 255 = do not use value
    position.navigation.satellitesUsed = satellitesUsed;
    context.publishSatelliteUsage(satellitesUsed);

    position.navigation.latitudeDegrees = pvt.latitude * GPSProtocolMath::RAD_TO_DEG;
    position.navigation.longitudeDegrees = pvt.longitude * GPSProtocolMath::RAD_TO_DEG;
    position.navigation.altitudeEllipsoidMeters = pvt.height;
    position.navigation.altitudeMslMeters = pvt.height - static_cast<double>(pvt.undulation);

    // Accuracy is reported as 2DRMS in cm; halve it for the RMS convention used by the other drivers.
    position.navigation.horizontalAccuracyMeters =
        pvt.hAccuracy != UINT16_MAX ? static_cast<float>(pvt.hAccuracy) / 200.0f : NAN;
    position.navigation.verticalAccuracyMeters =
        pvt.vAccuracy != UINT16_MAX ? static_cast<float>(pvt.vAccuracy) / 200.0f : NAN;

    position.navigation.speedMetersPerSecond = sqrtf(pvt.vn * pvt.vn + pvt.ve * pvt.ve);
    position.navigation.courseRadians = std::isfinite(pvt.cog) && pvt.cog >= 0.0f && pvt.cog <= 360.0f
                                            ? static_cast<float>(pvt.cog * GPSProtocolMath::DEG_TO_RAD)
                                            : NAN;

    // WNc/TOW is GNSS system time, not UTC. Without receiver UTC/leap information,
    // retain the epoch key internally and let the facade use reception UTC.
    position.navigation.utcTimeUs = 0;
    position.navigation.timestampUs = context.nowUs();
    return coordinatesValid;
}

void Protocol::_publishSurveyStatus(const SBF::PVTGeodetic& pvt, const GPSDecodedPosition& position,
                                    bool coordinatesValid, GPSDecodeContext& context)
{
    // Mode bit 6 means automatic base determination is still in progress, not completed.
    // Septentrio PolaRx5TR 5.5.0 Reference Guide, SBF Mode definition (p. 382).
    const bool active = !std::holds_alternative<GPSBaseStationConfig::Fixed>(_base.mode) && pvt.modeAutoSet();
    const bool valid = !pvt.modeAutoSet() && pvt.modeType() == 3 && !pvt.mode2D() && !pvt.error && coordinatesValid;
    // The final update on the active-to-inactive transition freezes the survey duration.
    if (active || _surveyActive) {
        (void) _surveyClock.update(context.nowUs());
    }
    _surveyActive = active;
    // PVT accuracy describes the navigation solution, not the averaged base survey.
    GPSEllipsoidPosition surveyPosition;
    if (coordinatesValid && !pvt.error) {
        surveyPosition = {.latitudeDegrees = position.navigation.latitudeDegrees,
                          .longitudeDegrees = position.navigation.longitudeDegrees,
                          .altitudeMeters = position.navigation.altitudeEllipsoidMeters};
    }
    context.publishSurvey(active, valid, _surveyClock.duration(), surveyPosition);
}

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind == GPSFrameKind::ASCIILine) {
        return text.starts_with("$R:") || text.starts_with("$R?") ? QLatin1StringView("Septentrio command replies")
                                                                  : QLatin1StringView();
    }
    // A candidate the framer truncated at its capture limit cannot be checked, so only short blocks count.
    if (frame.kind != GPSFrameKind::SBF) {
        return {};
    }
    const auto header = SBF::checkedHeader(frame.bytes);
    return header && header->length % 4 == 0 ? QLatin1StringView("SBF blocks") : QLatin1StringView();
}

/// The prompt Protocol::configure() reads the connection descriptor from, without forcing command input first.
bool Protocol::probe(GPSCommandChannel& channel)
{
    const auto scope = channel.deadlineScope(SBF::Plan::PROMPT_TIMEOUT);
    const QByteArrayView prompt = SBF::Plan::PROMPT;
    channel.beginCommand({prompt.toByteArray(), SBF::Plan::PROMPT_TIMEOUT});
    QByteArray port;
    const bool answered = detectPort(channel, port);
    channel.finishCommand(answered           ? GPSCommandOutcome::Acknowledged
                          : channel.failed() ? channel.failureOutcome()
                                             : GPSCommandOutcome::TimedOut);
    return answered;
}

}  // namespace

namespace SBF {

const GPSReceiverFamily FAMILY{
    .type = GPSType::septentrio,
    .logCategory = &SBFProtocolLog,
    .stream = {.framers = GPSFrameKind::SBF | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::SBF},
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace SBF
