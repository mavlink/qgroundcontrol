#include <algorithm>
#include <cmath>
#include <limits>

#include "GPSDriverQuectel.h"
#include "QuectelCodec_p.h"

namespace {
constexpr uint64_t STATUS_MAX_AGE_US = 5000000;
constexpr unsigned GPS_WEEK_MS = 604800000;

using QuectelCodec::Fields;
using QuectelCodec::number;
}  // namespace

int GPSNativeQuectel::handleReceiverLine(std::string_view line)
{
    const auto body = QuectelCodec::checkedBody(line);
    if (body.empty()) {
        return 0;
    }
    if (_replyHandler && _reply == GPSCommandOutcome::Pending) {
        _reply = _replyHandler(body);
    }
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
                if (_surveyPhase == SurveyPhase::AwaitingBoot) {
                    _surveyPhase = SurveyPhase::Verifying;
                }
            } else if (!_expectingBoot && (_configured || _surveyPhase != SurveyPhase::Off)) {
                _configured = false;
                _surveyPhase = SurveyPhase::Off;
                _revokeSurvey();
                controlFailed();
            }
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
    }
    return 0;
}

int GPSNativeQuectel::decodeByte(uint8_t byte)
{
    _expireSurvey();
    return GPSAsciiProtocol::decodeByte(byte);
}

void GPSNativeQuectel::flushDecoded()
{
    _expireSurvey();
    GPSAsciiProtocol::flushDecoded();
}

void GPSNativeQuectel::_revokeSurvey()
{
    setRTCMEnabled(false);
    if (_surveyReport) {
        _surveyReport.reset();
        GPSNativeSurveyReport report{};
        report.latitude = NAN;
        report.longitude = NAN;
        report.altitude = NAN;
        surveyInStatus(report);
    }
}

void GPSNativeQuectel::_expireSurvey()
{
    if (_surveyReport && (ioError() || nowUs() - _lastSurveyUs > STATUS_MAX_AGE_US)) {
        _revokeSurvey();
    }
}

void GPSNativeQuectel::_publishSurvey()
{
    _expireSurvey();
    if (_surveyPhase == SurveyPhase::Monitoring && _surveyReport) {
        // A status buffered during boot verification keeps its original receipt time.
        _decoded.events.emplace_back(*_surveyReport);
        setRTCMEnabled(_configured && (_surveyReport->flags & 1));
    }
}

bool GPSNativeQuectel::_handleSurvey(std::string_view body)
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
    if (_lastSurveyTow) {
        // TOW is a modular measurement clock, not a receipt timestamp. A late packet
        // from before Sunday's rollover must not look newer than the current week.
        const unsigned advance = (tow + GPS_WEEK_MS - *_lastSurveyTow) % GPS_WEEK_MS;
        if (advance == 0 || advance >= GPS_WEEK_MS / 2) {
            return false;
        }
    }
    _lastSurveyTow = tow;
    if ((_surveyPhase != SurveyPhase::Verifying && _surveyPhase != SurveyPhase::Monitoring) ||
        _outputMode != OutputMode::RTCM) {
        return false;
    }
    if (!number(reply[3], validity) || validity > 2 || !reply[4].empty() || !number(reply[6], observations) ||
        observations > 86400 || !number(reply[7], configuredCount) || configuredCount > 86400 ||
        !number(reply[8], ecef.x) || !number(reply[9], ecef.y) || !number(reply[10], ecef.z) ||
        !number(reply[11], accuracy) || accuracy < 0 || accuracy > std::numeric_limits<uint32_t>::max() / 1000.0) {
        _revokeSurvey();
        return false;
    }
    const bool matches = _baseConfig.useFixedBase ? configuredCount == 0 && observations == 0
                                                  : configuredCount == _baseConfig.surveyInDurationSecs;
    const double radius = std::hypot(ecef.x, ecef.y, ecef.z);
    const bool coordinatesKnown = radius >= 6000000 && radius <= 7000000;
    if (!matches || (!coordinatesKnown && (validity == 2 || (validity == 1 && observations != 0)))) {
        _revokeSurvey();
        return false;
    }
    if (_baseConfig.useFixedBase && validity == 2 &&
        (std::abs(ecef.x - _fixedECEF.x) > 0.001 || std::abs(ecef.y - _fixedECEF.y) > 0.001 ||
         std::abs(ecef.z - _fixedECEF.z) > 0.001)) {
        _revokeSurvey();
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid = validity == 2 && (_baseConfig.useFixedBase || observations >= configuredCount);
    GPSNativeSurveyReport report{};
    report.latitude = std::numeric_limits<double>::quiet_NaN();
    report.longitude = std::numeric_limits<double>::quiet_NaN();
    report.altitude = std::numeric_limits<float>::quiet_NaN();
    report.altitudeDatum = GPSNativeSurveyReport::AltitudeDatum::Ellipsoid;
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    report.accuracyKnown = !_baseConfig.useFixedBase && validity != 0 && observations != 0 && coordinatesKnown;
    report.mean_accuracy = report.accuracyKnown ? static_cast<uint32_t>(std::llround(accuracy * 1000)) : 0;
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.duration = observations;
    report.flags = valid ? 1 : validity == 1 ? 2 : 0;
    if (coordinatesKnown && validity != 0) {
        const auto position = fromEcef(ecef);
        report.latitude = position.latitudeDegrees;
        report.longitude = position.longitudeDegrees;
        report.altitude = position.altitudeMeters;
    }
    _lastSurveyUs = nowUs();
    report.timestamp = _lastSurveyUs;
    _surveyReport = report;
    _publishSurvey();
    return true;
}

int GPSNativeQuectel::receive(unsigned timeout)
{
    _expireSurvey();
    // Re-evaluate stale status before each transport read, even when one caller gives a large timeout.
    const int result = GPSAsciiProtocol::receive(std::min(timeout, 1000U));
    if (ioError()) {
        _configured = false;
        _surveyPhase = SurveyPhase::Off;
        _revokeSurvey();
        consume({});
    }
    return result;
}
