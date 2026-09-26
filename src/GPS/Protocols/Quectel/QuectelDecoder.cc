#include "Quectel/QuectelDecoder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <variant>

#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSFrame.h"
#include "NMEAFields.h"
#include "Quectel/QuectelCodec_p.h"

namespace {
constexpr unsigned GPS_WEEK_MS = 604800000;

using QuectelCodec::Fields;
using QuectelCodec::number;
}  // namespace

namespace Quectel {

Decoder::Decoder(bool satelliteInfoEnabled)
    : _nmea(GPSNMEAStream::Navigation::StandardNMEA, satelliteInfoEnabled)
{
    _nmea.setRTCMEnabled(false);
}

void Decoder::endSession(GPSDecodeContext& context)
{
    _configured = false;
    _phase = SurveyPhase::Off;
    _revokeSurvey(context);
}

void Decoder::startSession(GPSStreamDemux& stream)
{
    _lastTow.reset();
    _survey.reset();
    _phase = SurveyPhase::Off;
    _firmware.clear();
    _nmea.reset(stream);
    _nmea.setRTCMEnabled(false);
}

void Decoder::setBase(const GPSBaseStationConfig::Mode& mode)
{
    _baseMode = mode;
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&mode)) {
        _fixedECEF = GPSProtocolMath::toEcef(fixed->position);
    }
}

void Decoder::beginRestart(GPSDecodeContext& context)
{
    _revokeSurvey(context);
    _phase = SurveyPhase::AwaitingBoot;
}

void Decoder::watchBoot(GPSStreamDemux& stream)
{
    _nmea.reset(stream);
    _expectingBoot = true;
    _sawBoot = false;
    _restartRejected = false;
}

void Decoder::monitorSurvey(GPSDecodeContext& context)
{
    _phase = SurveyPhase::Monitoring;
    _publishSurvey(context);
}

void Decoder::finishSession(GPSDecodeContext& context)
{
    _configured = true;
    expireSurvey(context);
    _nmea.setRTCMEnabled(_survey && _survey->survey.valid);
}

GPSReceiveUpdates Decoder::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        return _nmea.decodeRTCM(frame, context);
    }
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    GPSReceiveUpdates updates = _nmea.decodeStandard(frame.text(), context);
    updates |= _decodeLine(frame.text(), context);
    return _nmea.finishLine(updates, context);
}

void Decoder::flush(GPSDecodeContext& context)
{
    expireSurvey(context);
    _nmea.flush(context);
}

GPSReceiveUpdates Decoder::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    const auto body = QuectelCodec::checkedBody(line);
    if (body.empty()) {
        return {};
    }
    context.offerReply(body);
    if (_expectingBoot && body.starts_with("PQTMSRR,") && QuectelCodec::rejected(Fields(body), "PQTMSRR")) {
        _restartRejected = true;
    }
    if (body.starts_with("PQTMSVINSTATUS,")) {
        return _handleSurvey(body, context) ? GPSReceiveUpdates(GPSReceiveUpdate::Activity) : GPSReceiveUpdates{};
    } else if (body.starts_with("PQTMVER,")) {
        const Fields reply(body);
        const std::string_view firmware(_firmware.constData(), static_cast<size_t>(_firmware.size()));
        if (reply.size() == 6 && reply[1] == "1" && reply[2] == "MODULE" && reply[3] == firmware) {
            // §2.3.1: this is the first output upon each successful startup.
            if (_expectingBoot && !_sawBoot) {
                _sawBoot = true;
                if (_phase == SurveyPhase::AwaitingBoot) {
                    _phase = SurveyPhase::Verifying;
                }
            } else if (!_expectingBoot && (_configured || _phase != SurveyPhase::Off)) {
                _configured = false;
                _phase = SurveyPhase::Off;
                _revokeSurvey(context);
                context.failControl();
            }
            return GPSReceiveUpdate::Activity;
        }
    }
    return {};
}

void Decoder::_revokeSurvey(GPSDecodeContext& context)
{
    _nmea.setRTCMEnabled(false);
    if (_survey) {
        _survey.reset();
        GPSDecodedSurvey report{};
        context.sink().publishSurvey(report);
    }
}

void Decoder::expireSurvey(GPSDecodeContext& context)
{
    if (_survey && (context.failed() || context.nowUs() - _survey->timestamp > STATUS_MAX_AGE_US)) {
        _revokeSurvey(context);
    }
}

void Decoder::_publishSurvey(GPSDecodeContext& context)
{
    expireSurvey(context);
    if (_phase == SurveyPhase::Monitoring && _survey) {
        // A status buffered during boot verification keeps its original receipt time.
        context.sink().publishAsReceived(*_survey);
        _nmea.setRTCMEnabled(_configured && _survey->survey.valid);
    }
}

bool Decoder::_handleSurvey(std::string_view body, GPSDecodeContext& context)
{
    const Fields reply(body);
    unsigned version = 0;
    unsigned tow = 0;
    unsigned validity = 0;
    unsigned observations = 0;
    unsigned configuredCount = 0;
    double accuracy = 0;
    GPSProtocolMath::Ecef ecef;
    if (reply.size() != 12 || !number(reply[1], version) || version != 1 || !number(reply[2], tow) ||
        tow >= GPS_WEEK_MS) {
        return false;
    }
    if (_lastTow) {
        // TOW is a modular measurement clock, not a receipt timestamp. A late packet
        // from before Sunday's rollover must not look newer than the current week.
        const unsigned advance = (tow + GPS_WEEK_MS - *_lastTow) % GPS_WEEK_MS;
        if (advance == 0 || advance >= GPS_WEEK_MS / 2) {
            return false;
        }
    }
    _lastTow = tow;
    if (_phase != SurveyPhase::Verifying && _phase != SurveyPhase::Monitoring) {
        return false;
    }
    if (!number(reply[3], validity) || validity > 2 || !reply[4].empty() || !number(reply[6], observations) ||
        observations > 86400 || !number(reply[7], configuredCount) || configuredCount > 86400 ||
        !number(reply[8], ecef.x) || !number(reply[9], ecef.y) || !number(reply[10], ecef.z) ||
        !number(reply[11], accuracy) || accuracy < 0 || accuracy > std::numeric_limits<uint32_t>::max() / 1000.0) {
        _revokeSurvey(context);
        return false;
    }
    const bool fixed = std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseMode);
    const bool matches = fixed
                             ? configuredCount == 0 && observations == 0
                             : configuredCount == std::get<GPSBaseStationConfig::SurveyIn>(_baseMode).duration.count();
    const double radius = std::hypot(ecef.x, ecef.y, ecef.z);
    const bool coordinatesKnown = radius >= 6000000 && radius <= 7000000;
    if (!matches || (!coordinatesKnown && (validity == 2 || (validity == 1 && observations != 0)))) {
        _revokeSurvey(context);
        return false;
    }
    if (fixed && validity == 2 &&
        (std::abs(ecef.x - _fixedECEF.x) > 0.001 || std::abs(ecef.y - _fixedECEF.y) > 0.001 ||
         std::abs(ecef.z - _fixedECEF.z) > 0.001)) {
        _revokeSurvey(context);
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid = validity == 2 && (fixed || observations >= configuredCount);
    GPSDecodedSurvey report{};
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    if (!fixed && validity != 0 && observations != 0 && coordinatesKnown) {
        report.survey.meanAccuracyMeters = static_cast<double>(std::llround(accuracy * 1000)) / 1000.0;
    }
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.survey.duration = std::chrono::seconds(observations);
    report.survey.valid = valid;
    report.survey.active = validity == 1 && !valid;
    if (coordinatesKnown && validity != 0) {
        report.survey.position = GPSProtocolMath::fromEcef(ecef);
    }
    report.timestamp = context.nowUs();
    _survey = report;
    _publishSurvey(context);
    return true;
}

}  // namespace Quectel

namespace QuectelCodec {

QByteArray frame(QByteArrayView body)
{
    const auto checksum = NMEA::checksum({body.data(), static_cast<size_t>(body.size())});
    QByteArray result;
    result.reserve(body.size() + 6);
    result.append('$').append(body).append('*');
    result.append(NMEAFields::hexDigit(checksum >> 4)).append(NMEAFields::hexDigit(checksum)).append("\r\n");
    return result;
}

std::string_view checkedBody(std::string_view line)
{
    const auto wire = NMEA::frame(line);
    return wire && wire->hasValidChecksum() ? wire->body : std::string_view{};
}

bool rejected(const Fields& reply, std::string_view command)
{
    unsigned error = 0;
    return reply.size() == 3 && reply[0] == command && reply[1] == "ERROR" && number(reply[2], error);
}

GPSCommandOutcome acknowledgement(const Fields& reply, std::string_view command)
{
    if (reply.size() == 2 && reply[0] == command && reply[1] == "OK") {
        return GPSCommandOutcome::Acknowledged;
    }
    return rejected(reply, command) ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Pending;
}

GPSCommandOutcome readback(const Fields& reply, std::string_view command, bool matches)
{
    if (rejected(reply, command)) {
        return GPSCommandOutcome::Rejected;
    }
    // Overflow retains the header, but can never verify a truncated readback.
    if ((reply.size() > 2 || reply.overflowed()) && reply[0] == command && reply[1] == "OK") {
        return !reply.overflowed() && matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    }
    return GPSCommandOutcome::Pending;
}

}  // namespace QuectelCodec
