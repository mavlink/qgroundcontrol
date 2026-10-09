#include <algorithm>
#include <chrono>
#include <optional>
#include <string_view>
#include <variant>

#include <QtCore/QString>

#include "Femto/FemtoPlan.h"
#include "GPSASCIILog.h"
#include "GPSCommandChannel.h"
#include "GPSNMEAFamilyProtocol.h"
#include "GPSReceiverFamilies.h"
#include "GPSStreamDemux.h"
#include "GPSSurveyClock.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FemtoProtocolLog, "GPS.Protocols.Femto")

namespace {

/// Configures a Femtomes receiver from Femto::Plan at the rate selected or detected, else Plan::BAUD: stops its logs,
/// identifies it and sets up the base station. A survey-in starts receiver position averaging; RTCM output follows
/// once decoding sees it finish, or at once for a fixed base. A rejected base command leaves a description in the
/// channel's error detail. Decodes RTCM3 frames, and standard NMEA through GPSNMEAStream, whose GGA also shows when a
/// survey-in finishes.
class Protocol final : public GPSNMEAFamilyProtocol
{
public:
    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    bool probe(GPSCommandChannel& channel) override;

    /// Starts RTCM output once decoding saw position averaging finish.
    void serviceStreaming(GPSCommandChannel& channel) override;

private:
    /// Completes a survey-in from a fixed-quality GGA and advances its duration; a malformed GGA does neither.
    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) override;
    void _setUpBase(GPSCommandChannel& channel);
    void _activateRTCMOutput(GPSCommandChannel& channel);

    GPSBaseStationConfig _base{};
    bool _rtcmOutputActive = false;
    /// Position averaging finished, so RTCM output is due.
    bool _rtcmActivationPending = false;
    GPSSurveyClock _surveyClock;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _ready = false;
    _rtcmOutputActive = false;
    _rtcmActivationPending = false;
    _surveyClock = {};
    // Corrections are only framed from a receiver that answered.
    GPSStreamDemux& stream = channel.stream();
    stream.setEnabled(GPSFrameKind::RTCM3, false);
    _nmea.setRTCMEnabled(false);
    _nmea.reset(stream);
    _base = config.base;
    // No command depends on the rate, so the link keeps the one selected or detected; otherwise it runs at Plan::BAUD.
    baud = config.linkBaud(baud, Femto::Plan::BAUD);
    (void) channel.setBaudrate(baud);
    auto identity = channel.runSequence(Femto::Plan::sequence(Femto::Plan::IDENTIFY));
    for (unsigned round = 1; round < Femto::Plan::IDENTIFY_ROUNDS && !identity.succeeded(); ++round) {
        identity = channel.runSequence(Femto::Plan::sequence(Femto::Plan::IDENTIFY));
    }
    if (!identity.succeeded()) {
        if (identity.outcome == GPSCommandOutcome::Rejected) {
            channel.failSequence(identity);
        } else {
            channel.failNoAnswer({&baud, 1});
        }
        return false;
    }

    stream.reset(GPSFrameKind::NMEASentence);
    stream.setEnabled(GPSFrameKind::RTCM3, true);
    _nmea.setRTCMEnabled(true);
    _setUpBase(channel);
    _ready = !channel.failed();
    return _ready;
}

void Protocol::serviceStreaming(GPSCommandChannel& channel)
{
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        _activateRTCMOutput(channel);
    }
}

void Protocol::_setUpBase(GPSCommandChannel& channel)
{
    GPSDecodeContext& context = channel.context();
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode);
    if (!fixed) {
        if (!channel.runRequired(Femto::Plan::sequence(Femto::Plan::SURVEY_IN))) {
            return;
        }
        _surveyClock.start(context);
        return;
    }
    if (!channel.runRequired(Femto::Plan::fixedBase(fixed->position))) {
        return;
    }
    _activateRTCMOutput(channel);
    if (_rtcmOutputActive) {
        context.publishSurvey(false, true, {}, fixed->position);
    }
}

void Protocol::_activateRTCMOutput(GPSCommandChannel& channel)
{
    if (channel.runRequired(Femto::Plan::sequence(Femto::Plan::RTCM_OUTPUT))) {
        _rtcmOutputActive = true;
    }
}

GPSReceiveUpdates Protocol::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    if (line.size() >= 6 && line.substr(3, 3) == "GGA") {
        const auto sentence = NMEA::sentence(line);
        const auto fix = sentence ? NMEA::gga(*sentence) : std::nullopt;
        if (!fix) {
            return {};
        }
        // Only a survey started by this configuration may complete it; earlier GGA output is stale.
        if (!_rtcmOutputActive && _surveyClock.running() && fix->quality == Femto::Plan::AVERAGED_POSITION_QUALITY) {
            _surveyClock.finish(context, true,
                                {.latitudeDegrees = fix->latitude,
                                 .longitudeDegrees = fix->longitude,
                                 .altitudeMeters = fix->altitude + fix->geoidSeparation});
            _rtcmActivationPending = true;
        }
    }
    _surveyClock.publishProgress(context);
    return {};
}

bool portName(std::string_view field)
{
    const size_t digits = field.find_first_of("0123456789");
    if (digits == 0 || digits == std::string_view::npos) {
        return false;
    }
    return std::ranges::all_of(field.substr(0, digits), NMEA::isAsciiUpper) &&
           std::ranges::all_of(field.substr(digits), NMEA::isAsciiDigit);
}

/// Femtomes receivers speak the NovAtel command set: abbreviated "<... OK" replies, and ASCII logs whose header names
/// the port, unlike Unicore's.
QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    if (text.starts_with('<') && (text.ends_with(" OK") || text.starts_with(Femto::Plan::REJECTED))) {
        return QLatin1StringView("abbreviated ASCII replies");
    }
    const auto log = gpsASCIILog(text);
    return log && portName(log->source) ? QLatin1StringView("NovAtel-format ASCII logs") : QLatin1StringView();
}

/// The read-only half of Plan::IDENTIFY, in as many rounds: UNLOGALL would stop the receiver's logs.
bool Protocol::probe(GPSCommandChannel& channel)
{
    bool identified = false;
    for (unsigned round = 0; round < Femto::Plan::IDENTIFY_ROUNDS && !identified && !channel.failed(); ++round) {
        identified = channel.runSequence(Femto::Plan::sequence({&Femto::Plan::VERSION, 1})).succeeded();
    }
    return identified;
}

}  // namespace

namespace Femto {

const GPSReceiverFamily FAMILY{
    .type = GPSType::femto,
    .logCategory = &FemtoProtocolLog,
    // RTCM3 is framed once the receiver answered (see Protocol::configure()).
    .stream = {.framers = GPSFrameKind::NMEASentence | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::NMEASentence},
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace Femto
