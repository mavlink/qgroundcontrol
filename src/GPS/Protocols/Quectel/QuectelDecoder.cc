#include <algorithm>
#include <cmath>
#include <limits>

#include "NMEAFields.h"
#include "QuectelCodec_p.h"
#include "QuectelProtocol.h"

namespace {
constexpr uint64_t STATUS_MAX_AGE_US = 5000000;
constexpr unsigned GPS_WEEK_MS = 604800000;

using QuectelCodec::Fields;
using QuectelCodec::number;
}  // namespace

int QuectelProtocol::handleReceiverLine(std::string_view line)
{
    const auto body = QuectelCodec::checkedBody(line);
    if (body.empty()) {
        return 0;
    }
    offerReply(body);
    if (_expectingBoot && body.starts_with("PQTMSRR,") && QuectelCodec::rejected(Fields(body), "PQTMSRR")) {
        _restartRejected = true;
    }
    if (body.starts_with("PQTMSVINSTATUS,")) {
        return _handleSurvey(body) ? GPSDecodedBatch::PROTOCOL_ACTIVITY : 0;
    } else if (body.starts_with("PQTMVER,")) {
        const Fields reply(body);
        if (reply.size() == 6 && reply[1] == "1" && reply[2] == "MODULE" && reply[3] == _firmware) {
            // §2.3.1: this is the first output upon each successful startup.
            if (_expectingBoot && !_sawBoot) {
                _sawBoot = true;
                if (_survey.phase == SurveyPhase::AwaitingBoot) {
                    _survey.phase = SurveyPhase::Verifying;
                }
            } else if (!_expectingBoot && (_configured || _survey.phase != SurveyPhase::Off)) {
                _configured = false;
                _survey.phase = SurveyPhase::Off;
                _revokeSurvey();
                controlFailed();
            }
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
    }
    return 0;
}

int QuectelProtocol::decodeByte(uint8_t byte)
{
    _expireSurvey();
    return GPSAsciiProtocol::decodeByte(byte);
}

void QuectelProtocol::flushDecoded()
{
    _expireSurvey();
    GPSAsciiProtocol::flushDecoded();
}

void QuectelProtocol::_revokeSurvey()
{
    setRTCMEnabled(false);
    if (_survey.report) {
        _survey.report.reset();
        GPSDecodedSurvey report{};
        publishSurvey(report);
    }
}

void QuectelProtocol::_expireSurvey()
{
    if (_survey.report && (hasIOError() || nowUs() - _survey.report->timestamp > STATUS_MAX_AGE_US)) {
        _revokeSurvey();
    }
}

void QuectelProtocol::_publishSurvey()
{
    _expireSurvey();
    if (_survey.phase == SurveyPhase::Monitoring && _survey.report) {
        // A status buffered during boot verification keeps its original receipt time.
        _decoded.events.emplace_back(*_survey.report);
        setRTCMEnabled(_configured && _survey.report->survey.valid);
    }
}

bool QuectelProtocol::_handleSurvey(std::string_view body)
{
    const Fields reply(body);
    unsigned version = 0;
    unsigned tow = 0;
    unsigned validity = 0;
    unsigned observations = 0;
    unsigned configuredCount = 0;
    double accuracy = 0;
    EcefMeters ecef;
    if (reply.size() != 12 || !number(reply[1], version) || version != 1 || !number(reply[2], tow) ||
        tow >= GPS_WEEK_MS) {
        return false;
    }
    if (_survey.lastTow) {
        // TOW is a modular measurement clock, not a receipt timestamp. A late packet
        // from before Sunday's rollover must not look newer than the current week.
        const unsigned advance = (tow + GPS_WEEK_MS - *_survey.lastTow) % GPS_WEEK_MS;
        if (advance == 0 || advance >= GPS_WEEK_MS / 2) {
            return false;
        }
    }
    _survey.lastTow = tow;
    if (_survey.phase != SurveyPhase::Verifying && _survey.phase != SurveyPhase::Monitoring) {
        return false;
    }
    if (!number(reply[3], validity) || validity > 2 || !reply[4].empty() || !number(reply[6], observations) ||
        observations > 86400 || !number(reply[7], configuredCount) || configuredCount > 86400 ||
        !number(reply[8], ecef.x) || !number(reply[9], ecef.y) || !number(reply[10], ecef.z) ||
        !number(reply[11], accuracy) || accuracy < 0 || accuracy > std::numeric_limits<uint32_t>::max() / 1000.0) {
        _revokeSurvey();
        return false;
    }
    const bool matches =
        std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)
            ? configuredCount == 0 && observations == 0
            : configuredCount == std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs;
    const double radius = std::hypot(ecef.x, ecef.y, ecef.z);
    const bool coordinatesKnown = radius >= 6000000 && radius <= 7000000;
    if (!matches || (!coordinatesKnown && (validity == 2 || (validity == 1 && observations != 0)))) {
        _revokeSurvey();
        return false;
    }
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) && validity == 2 &&
        (std::abs(ecef.x - _fixedECEF.x) > 0.001 || std::abs(ecef.y - _fixedECEF.y) > 0.001 ||
         std::abs(ecef.z - _fixedECEF.z) > 0.001)) {
        _revokeSurvey();
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid = validity == 2 && (std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ||
                                         observations >= configuredCount);
    GPSDecodedSurvey report{};
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) && validity != 0 && observations != 0 &&
        coordinatesKnown) {
        report.survey.meanAccuracyMeters = static_cast<double>(std::llround(accuracy * 1000)) / 1000.0;
    }
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.survey.duration = std::chrono::seconds(observations);
    report.survey.valid = valid;
    report.survey.active = validity == 1 && !valid;
    if (coordinatesKnown && validity != 0) {
        const auto position = fromEcef(ecef);
        report.survey.position = position;
    }
    report.timestamp = nowUs();
    _survey.report = report;
    _publishSurvey();
    return true;
}

int QuectelProtocol::receive(unsigned timeout)
{
    _expireSurvey();
    // Re-evaluate stale status before each transport read, even when one caller gives a large timeout.
    const int result = GPSAsciiProtocol::receive(std::min(timeout, 1000U));
    if (hasIOError()) {
        _configured = false;
        _survey.phase = SurveyPhase::Off;
        _revokeSurvey();
        consume({});
    }
    return result;
}

namespace QuectelCodec {

std::string frame(std::string_view body)
{
    const auto checksum = NMEA::checksum(body);
    std::string result{"$"};
    result.append(body);
    result.push_back('*');
    result.push_back(NMEAFields::hexDigit(checksum >> 4));
    result.push_back(NMEAFields::hexDigit(checksum));
    result.append("\r\n");
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
