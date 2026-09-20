#include "GPSDriverQuectel.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>
#include <vector>

#include <QtCore/QScopeGuard>

#include "NMEA/NMEASentence.h"

namespace {
// Quectel LG290P(03)&LGx80P(03) GNSS Protocol Specification V1.1:
// §§2.3.9, 2.3.15, 2.3.22–25, 2.3.28. Base Station Mode Application Note
// V1.1 §3.2 explicitly permits re-executing an unchanged survey and rebooting without saving.
constexpr unsigned CONFIGURATION_TIMEOUT_MS = 45000;
constexpr unsigned RESTART_TIMEOUT_MS = 8000;
constexpr uint64_t STATUS_MAX_AGE_US = 5000000;
constexpr unsigned GPS_WEEK_MS = 604800000;

using Fields = std::vector<std::string_view>;

Fields fields(std::string_view body)
{
    Fields result;
    while (true) {
        const auto comma = body.find(',');
        result.push_back(body.substr(0, comma));
        if (comma == std::string_view::npos) {
            return result;
        }
        body.remove_prefix(comma + 1);
    }
}

template <typename T>
bool number(std::string_view text, T& result)
{
    if (text.empty() || text.front() == '+') {
        return false;
    }
    const auto parsed = NMEA::number<T>(text);
    if (!parsed) {
        return false;
    }
    result = *parsed;
    return true;
}

int hexDigit(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    return -1;
}

std::string frame(std::string_view body)
{
    uint8_t checksum = 0;
    for (const unsigned char byte : body) {
        checksum ^= byte;
    }
    constexpr char HEX[] = "0123456789ABCDEF";
    return "$" + std::string(body) + '*' + HEX[checksum >> 4] + HEX[checksum & 15] + "\r\n";
}

std::string_view checkedBody(std::string_view line)
{
    if (line.size() < 5 || line.front() != '$' || line[line.size() - 3] != '*') {
        return {};
    }
    const int high = hexDigit(line[line.size() - 2]);
    const int low = hexDigit(line.back());
    if (high < 0 || low < 0) {
        return {};
    }
    const auto body = line.substr(1, line.size() - 4);
    uint8_t checksum = 0;
    for (const unsigned char byte : body) {
        if (byte < 32 || byte > 126 || byte == '$' || byte == '*') {
            return {};
        }
        checksum ^= byte;
    }
    return checksum == ((high << 4) | low) ? body : std::string_view{};
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
    if (reply.size() > 2 && reply[0] == command && reply[1] == "OK") {
        return matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    }
    return GPSCommandOutcome::Pending;
}
}  // namespace

GPSNativeQuectel::GPSNativeQuectel(GPSProtocolIO io, GPSNativePositionReport* position,
                                   GPSNativeSatelliteReport* satellites)
    : GPSAsciiProtocol(std::move(io), position, satellites)
{
    setRTCMEnabled(false);
}

GPSCommandOutcome GPSNativeQuectel::_transact(const std::string& command, ReplyHandler handler, unsigned timeoutMs)
{
    _reply = GPSCommandOutcome::Pending;
    _replyHandler = std::move(handler);
    const auto clearReply = qScopeGuard([this] { _replyHandler = {}; });
    const GPSConfigurationStep step{command, std::chrono::milliseconds(timeoutMs)};
    const std::string bytes = frame(command);
    if (!writeCommand(step, {reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()})) {
        return ioError() == ReadCancelled ? GPSCommandOutcome::Cancelled : GPSCommandOutcome::TransportError;
    }
    return awaitCommand(step, [this] { return _reply; }).outcome;
}

bool GPSNativeQuectel::_acknowledge(const std::string& command, unsigned timeoutMs)
{
    const std::string name = command.substr(0, command.find(','));
    return _transact(
               command,
               [name](std::string_view body) {
                   const auto reply = fields(body);
                   if (reply.size() == 2 && reply[0] == name && reply[1] == "OK") {
                       return GPSCommandOutcome::Acknowledged;
                   }
                   return rejected(reply, name) ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Pending;
               },
               timeoutMs) == GPSCommandOutcome::Acknowledged;
}

bool GPSNativeQuectel::_identify(unsigned timeoutMs)
{
    return _transact(
               "PQTMVERNO",
               [this](std::string_view body) {
                   const auto reply = fields(body);
                   if (rejected(reply, "PQTMVERNO")) {
                       return GPSCommandOutcome::Rejected;
                   }
                   if (reply.size() != 4 || reply[0] != "PQTMVERNO") {
                       return GPSCommandOutcome::Pending;
                   }
                   _firmware = reply[1];
                   // LG580P/LG680P use the same commands, but are not qualified by this driver.
                   return _firmware.starts_with("LG290P03") && _firmware.size() > 8
                              ? GPSCommandOutcome::ReadbackVerified
                              : GPSCommandOutcome::Rejected;
               },
               timeoutMs) == GPSCommandOutcome::ReadbackVerified;
}

bool GPSNativeQuectel::_verifyRole(bool requireMatch)
{
    const unsigned expected = _outputMode == OutputMode::RTCM ? 2 : 1;
    return _transact("PQTMCFGRCVRMODE,R", [this, expected, requireMatch](std::string_view body) {
               const auto reply = fields(body);
               unsigned mode = 0;
               const bool valid = reply.size() == 3 && reply[0] == "PQTMCFGRCVRMODE" && reply[1] == "OK" &&
                                  number(reply[2], mode) && mode <= 2;
               if (valid) {
                   _receiverRole = mode;
               }
               return readback(reply, "PQTMCFGRCVRMODE", valid && (!requireMatch || mode == expected));
           }) == GPSCommandOutcome::ReadbackVerified;
}

bool GPSNativeQuectel::_verifyBase(bool requireMatch)
{
    _baseMatches = false;
    if (_transact("PQTMCFGFIXRATE,R", [](std::string_view body) {
            const auto reply = fields(body);
            unsigned interval = 0;
            return readback(reply, "PQTMCFGFIXRATE",
                            reply.size() == 3 && number(reply[2], interval) && interval == 1000);
        }) != GPSCommandOutcome::ReadbackVerified) {
        return false;
    }
    return _transact("PQTMCFGSVIN,R", [this, requireMatch](std::string_view body) {
               const auto reply = fields(body);
               unsigned mode = 0;
               unsigned count = 0;
               double accuracy = 0;
               double distance = 0;
               std::array<double, 3> ecef{};
               const bool valid = (reply.size() == 8 || reply.size() == 9) && reply[0] == "PQTMCFGSVIN" &&
                                  reply[1] == "OK" && number(reply[2], mode) && mode <= 2 && number(reply[3], count) &&
                                  number(reply[4], accuracy) && number(reply[5], ecef[0]) &&
                                  number(reply[6], ecef[1]) && number(reply[7], ecef[2]) &&
                                  (reply.size() == 8 || number(reply[8], distance)) && count <= 86400 &&
                                  accuracy >= 0 && accuracy <= 1000 && distance >= 0 && distance <= 10;
               bool matches = valid;
               if (valid) {
                   _baseHasDistance = reply.size() == 9;
               }
               if (matches && _baseConfig.useFixedBase) {
                   matches = mode == 2 && count == 0 && accuracy == 0 && std::abs(ecef[0] - _fixedECEF[0]) <= 0.00011 &&
                             std::abs(ecef[1] - _fixedECEF[1]) <= 0.00011 &&
                             std::abs(ecef[2] - _fixedECEF[2]) <= 0.00011;
               } else if (matches) {
                   matches = mode == 1 && count == _baseConfig.surveyInDurationSecs &&
                             std::abs(accuracy - _baseConfig.surveyInAccMeters) <= 0.000000001 && distance == 0;
                   if (matches) {
                       // Re-execute precisely the read configuration, including otherwise ignored ECEF fields.
                       _surveyRestartCommand = "PQTMCFGSVIN,W";
                       for (size_t index = 2; index < reply.size(); ++index) {
                           _surveyRestartCommand += "," + std::string(reply[index]);
                       }
                   }
               }
               _baseMatches = matches;
               return readback(reply, "PQTMCFGSVIN", valid && (!requireMatch || matches));
           }) == GPSCommandOutcome::ReadbackVerified;
}

std::string GPSNativeQuectel::_baseCommand() const
{
    std::ostringstream command;
    command.imbue(std::locale::classic());
    command << std::fixed << "PQTMCFGSVIN,W,";
    if (_baseConfig.useFixedBase) {
        command << "2,0,0," << std::setprecision(4) << _fixedECEF[0] << ',' << _fixedECEF[1] << ',' << _fixedECEF[2];
    } else {
        command << "1," << _baseConfig.surveyInDurationSecs << ',' << std::setprecision(9)
                << _baseConfig.surveyInAccMeters << ",0,0,0";
    }
    if (_baseHasDistance) {
        command << ",0";
    }
    return command.str();
}

bool GPSNativeQuectel::_saveConfiguration()
{
    const bool saved = _acknowledge("PQTMSAVEPAR", 5000);
    if (saved) {
        _persistentSaveAcknowledged = true;
        _persistentSaveUncertain = false;
        log(GPSProtocolLogLevel::Debug, "LG290P acknowledged saving configuration to nonvolatile memory");
    } else {
        _persistentSaveUncertain = _reply != GPSCommandOutcome::Rejected && _commandWrite.acceptedBytes > 0;
    }
    return saved;
}

bool GPSNativeQuectel::_setMessageRate(std::string_view name, unsigned rate, std::string_view version)
{
    const std::string suffix = version.empty() ? "" : "," + std::string(version);
    if (!_acknowledge("PQTMCFGMSGRATE,W," + std::string(name) + "," + std::to_string(rate) + suffix)) {
        return false;
    }
    return _transact("PQTMCFGMSGRATE,R," + std::string(name) + suffix, [name, rate, version](std::string_view body) {
               auto reply = fields(body);
               // V1.0 documents an empty optional version after standard NMEA rates.
               if (version.empty() && reply.size() == 5 && reply.back().empty()) {
                   reply.pop_back();
               }
               unsigned value = 0;
               return readback(reply, "PQTMCFGMSGRATE",
                               reply.size() == (version.empty() ? 4 : 5) && reply[2] == name &&
                                   number(reply[3], value) && value == rate &&
                                   (version.empty() || reply[4] == version));
           }) == GPSCommandOutcome::ReadbackVerified;
}

bool GPSNativeQuectel::_restart(bool requireRoleMatch, bool startSurveySession)
{
    _revokeSurvey();
    _surveyPhase = startSurveySession ? SurveyPhase::AwaitingBoot : SurveyPhase::Off;
    log(GPSProtocolLogLevel::Debug, "Restarting LG290P and verifying its saved configuration");
    const Operation operation(*this, RESTART_TIMEOUT_MS);
    const uint64_t deadline = nowUs() + uint64_t(RESTART_TIMEOUT_MS) * 1000;
    const auto bytes = frame("PQTMSRR");
    beginCommandWrite("PQTMSRR");
    if (write(bytes.data(), static_cast<int>(bytes.size())) != static_cast<int>(bytes.size())) {
        return false;
    }
    // PQTMSRR has no documented ACK. Do not inflate successful transport completion to acknowledgment.
    failCommandWrite(GPSCommandOutcome::Written);
    resetStream();
    _expectingBoot = true;
    _sawBoot = false;
    _restartRejected = false;
    do {
        waitFor(std::chrono::milliseconds(200));
        if (ioError()) {
            break;
        }
        const bool identified = _identify(700);
        if (_restartRejected) {
            _ioErrorDetail = QStringLiteral("LG290P rejected PQTMSRR.");
            break;
        }
        if (identified && _sawBoot) {
            _expectingBoot = false;
            return _verifyRole(requireRoleMatch);
        }
    } while (!ioError() && nowUs() < deadline);
    _expectingBoot = false;
    return false;
}

int GPSNativeQuectel::_fail(const char* reason)
{
    _configured = false;
    _surveyPhase = SurveyPhase::Off;
    _revokeSurvey();
    consume({});
    QString detail = QString::fromUtf8(reason);
    if (!ioErrorDetail().isEmpty()) {
        detail += QStringLiteral(" ") + ioErrorDetail();
    }
    controlFailed();
    if (_persistentSaveAcknowledged || _persistentSaveUncertain) {
        const QString persistence =
            _persistentSaveAcknowledged ? QStringLiteral("An LG290P flash save was acknowledged. ") : QString{};
        const QString uncertain =
            _persistentSaveUncertain ? QStringLiteral("The latest flash save may have taken effect. ") : QString{};
        _ioErrorDetail = persistence + uncertain + detail +
                         QStringLiteral(" Receiver settings may have changed; no rollback was attempted.");
        log(GPSProtocolLogLevel::Warning, "%s", qPrintable(_ioErrorDetail));
    } else if (ioError() != ReadCancelled) {
        _ioErrorDetail = detail;
        log(GPSProtocolLogLevel::Warning, "%s", reason);
    }
    return ioError();
}

int GPSNativeQuectel::configure(unsigned& baud, const GPSConfig& config)
{
    resetIOError();
    _configured = false;
    _surveyPhase = SurveyPhase::Off;
    _revokeSurvey();
    consume({});
    _lastSurveyTow.reset();
    _persistentSaveAcknowledged = false;
    _persistentSaveUncertain = false;
    _firmware.clear();
    _surveyRestartCommand.clear();
    resetStream();
    setRTCMEnabled(false);
    if (!validateConfiguration(config, false, true) || config.gnss_systems != GNSSSystemsMask::RECEIVER_DEFAULTS ||
        (config.output_mode == OutputMode::RTCM &&
         (config.base.surveyMode != GPSBaseStationConfig::SurveyMode::AccuracyControlled ||
          (!config.base.useFixedBase &&
           (config.base.surveyInDurationSecs > 86400 || config.base.surveyInAccMeters > 1000))))) {
        return _fail(
            "Unsupported LG290P configuration: use native 1 Hz observation count (maximum 86400) and "
            "3D position accuracy threshold (maximum 1000 m), not receiver-managed survey");
    }
    const Operation operation(*this, CONFIGURATION_TIMEOUT_MS);
    _outputMode = config.output_mode;
    _baseConfig = config.base;
    if (_baseConfig.useFixedBase) {
        lla2ECEF(_baseConfig.fixedBaseLatitude, _baseConfig.fixedBaseLongitude, _baseConfig.fixedBaseAltitudeMeters,
                 _fixedECEF[0], _fixedECEF[1], _fixedECEF[2]);
    }
    bool identified = false;
    const std::array<unsigned, 6> candidates{460800, 115200, 230400, 921600, 57600, 9600};
    for (const unsigned candidate : candidates) {
        const unsigned selected = baud == 0 ? candidate : baud;
        if (setBaudrate(static_cast<int>(selected)) < 0) {
            return _fail("Cannot configure LG290P host serial speed");
        }
        resetStream();
        if (_identify()) {
            baud = selected;
            identified = true;
            break;
        }
        if (ioError() || !_firmware.empty() || baud != 0) {
            break;
        }
    }
    if (!identified) {
        return _fail("No verified LG290P(03) identity; receiver configuration was not changed");
    }
    log(GPSProtocolLogLevel::Debug, "Quectel receiver firmware: %s", _firmware.c_str());
    if (!_verifyRole(false)) {
        return _fail("LG290P receiver role query failed; no role change was attempted");
    }
    bool restarted = false;
    if (config.allowPersistentChanges) {
        // Work from saved settings, not another client's uncommitted changes. A role
        // readback alone cannot distinguish a pending role from the active one.
        if (!_restart(false, _outputMode == OutputMode::RTCM)) {
            return _fail("LG290P saved role query after restart failed; no persistent change was attempted");
        }
        restarted = true;
    }
    const unsigned expectedRole = _outputMode == OutputMode::RTCM ? 2 : 1;
    if (_receiverRole != expectedRole) {
        if (!config.allowPersistentChanges) {
            return _fail(
                "LG290P role mismatch: save the requested rover/base role externally, reboot and reconnect. "
                "Alternatively, explicitly allow persistent changes for this connection");
        }
        if (!_acknowledge("PQTMCFGRCVRMODE,W," + std::to_string(expectedRole)) || !_verifyRole()) {
            return _fail("LG290P role change was rejected or its readback did not match");
        }
        if (!_saveConfiguration()) {
            return _fail("LG290P role save failed; receiver activation is not verified");
        }
        if (!_restart(true, _outputMode == OutputMode::RTCM)) {
            return _fail("LG290P saved role could not be verified after restart");
        }
        restarted = true;
    }
    bool baseChanged = false;
    if (_outputMode == OutputMode::RTCM) {
        if (!_verifyBase(false)) {
            return _fail("LG290P base settings query failed; no base change was attempted");
        }
        if (!_baseMatches && !config.allowPersistentChanges) {
            return _fail(
                "LG290P base settings mismatch: provision/save the requested ECEF coordinates or survey "
                "observation count/accuracy externally, or explicitly allow persistent changes for this connection");
        }
        if (!_baseMatches) {
            if (!_acknowledge(_baseCommand()) || !_verifyBase()) {
                return _fail("LG290P base change was rejected or its readback did not match");
            }
            if (!_saveConfiguration()) {
                return _fail("LG290P base save failed; receiver activation is not verified");
            }
            if (!_restart(true, true) || !_verifyBase()) {
                return _fail("LG290P saved base settings could not be verified after restart");
            }
            baseChanged = true;
        }
        if (!_baseConfig.useFixedBase && !baseChanged) {
            log(GPSProtocolLogLevel::Debug,
                "LG290P survey uses accepted 1 Hz observations, not wall time; accuracy filters individual 3D fixes. "
                "The receiver itself stores converged coordinates");
            if (!_acknowledge(_surveyRestartCommand)) {
                return _fail("LG290P rejected restarting the unchanged, externally saved survey");
            }
            restarted = false;
        }
    }
    // A role readback can reflect an unsaved, not-yet-active change. Reboot and read back
    // the saved role before claiming that the receiver actually operates in that role.
    if ((!restarted && !_restart(true, _outputMode == OutputMode::RTCM)) ||
        (_outputMode == OutputMode::RTCM && !baseChanged && !_verifyBase())) {
        return _fail("LG290P restart/readback failed; saved role or base settings do not match the request");
    }
    if (_outputMode == OutputMode::RTCM) {
        _surveyPhase = SurveyPhase::Monitoring;
        _publishSurvey();
        if (!_setMessageRate("PQTMSVINSTATUS", 1, "1") || !_setMessageRate("RTCM3-1005", 1) ||
            !_setMessageRate("RTCM3-107X", 1, "0")) {
            return _fail("LG290P base message output configuration/readback failed");
        }
    }
    for (const std::string_view name : {"GGA", "GST", "GSA", "GSV"}) {
        if (!_setMessageRate(name, 1)) {
            return _fail("LG290P NMEA output configuration/readback failed");
        }
    }
    _configured = true;
    _expireSurvey();
    setRTCMEnabled(_surveyReport && (_surveyReport->flags & 1));
    return 0;
}

int GPSNativeQuectel::handleReceiverLine(std::string_view line)
{
    const auto body = checkedBody(line);
    if (body.empty()) {
        return 0;
    }
    if (_replyHandler && _reply == GPSCommandOutcome::Pending) {
        _reply = _replyHandler(body);
    }
    if (_expectingBoot && body.starts_with("PQTMSRR,") && rejected(fields(body), "PQTMSRR")) {
        _restartRejected = true;
    }
    if (body.starts_with("PQTMSVINSTATUS,")) {
        return _handleSurvey(body) ? GPSDecodedBatch::PROTOCOL_ACTIVITY : 0;
    } else if (body.starts_with("PQTMVER,")) {
        const auto reply = fields(body);
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
    const auto reply = fields(body);
    unsigned version = 0;
    unsigned tow = 0;
    unsigned validity = 0;
    unsigned observations = 0;
    unsigned configuredCount = 0;
    double accuracy = 0;
    std::array<double, 3> ecef{};
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
        !number(reply[8], ecef[0]) || !number(reply[9], ecef[1]) || !number(reply[10], ecef[2]) ||
        !number(reply[11], accuracy) || accuracy < 0 || accuracy > std::numeric_limits<uint32_t>::max() / 1000.0) {
        _revokeSurvey();
        return false;
    }
    const bool matches = _baseConfig.useFixedBase ? configuredCount == 0 && observations == 0
                                                  : configuredCount == _baseConfig.surveyInDurationSecs;
    const double radius = std::hypot(ecef[0], ecef[1], ecef[2]);
    const bool coordinatesKnown = radius >= 6000000 && radius <= 7000000;
    if (!matches || (!coordinatesKnown && (validity == 2 || (validity == 1 && observations != 0)))) {
        _revokeSurvey();
        return false;
    }
    if (_baseConfig.useFixedBase && validity == 2 &&
        (std::abs(ecef[0] - _fixedECEF[0]) > 0.001 || std::abs(ecef[1] - _fixedECEF[1]) > 0.001 ||
         std::abs(ecef[2] - _fixedECEF[2]) > 0.001)) {
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
        ECEF2lla(ecef[0], ecef[1], ecef[2], report.latitude, report.longitude, report.altitude);
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
