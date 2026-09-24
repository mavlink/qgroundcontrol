#include "GPSDriverUnicore.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

#include <QtCore/QScopeGuard>

#include "CRC32.h"

// Independent implementation of Unicore N4 Commands and Logs Reference Book, EN R1.6:
// https://en.unicore.com/uploads/file/Unicore%20Reference%20Commands%20Manual%20For%20N4%20High%20Precision%20Products_V2_EN_R1.6.pdf
// Sections 3, 7.3.1, 7.3.27, 7.3.44; Appendices 1/2 and position/solution status tables.
// Captured command-reply framing (XOR includes '$'):
// https://s-taka.org/control-command-for-gnss-receiver-um982/
namespace {

bool equalCommand(std::string_view left, std::string_view right)
{
    const auto upper = [](char ch) { return ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch; };
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), [&](char a, char b) { return upper(a) == upper(b); });
}

std::string_view unquote(std::string_view field)
{
    if (field.size() < 2 || field.front() != '"' || field.back() != '"') {
        return {};
    }
    field.remove_prefix(1);
    field.remove_suffix(1);
    return field.find('"') == std::string_view::npos ? field : std::string_view{};
}

bool validChecksum(std::string_view line, bool crc32)
{
    const auto star = line.find('*');
    const size_t digits = crc32 ? 8 : 2;
    if (star == std::string_view::npos || star + digits + 1 != line.size()) {
        return false;
    }
    for (const auto ch : line.substr(star + 1)) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
            return false;
        }
    }
    const auto expected = NMEA::number<uint32_t>(line.substr(star + 1), 16);
    if (!expected) {
        return false;
    }
    uint32_t checksum = 0;
    if (crc32) {
        // Reflected CRC-32, initial value zero and no final XOR; '#' is excluded.
        checksum = QGC::crc32Update({reinterpret_cast<const uint8_t*>(line.data() + 1), star - 1});
    } else {
        // MODE and command acknowledgments include their leading '#' / '$', unlike NMEA.
        checksum = NMEA::checksum(line.substr(0, star));
    }
    return checksum == *expected;
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
}  // namespace

GPSNativeUnicore::GPSNativeUnicore(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSAsciiProtocol(std::move(io), satelliteInfoEnabled)
{
    setRTCMEnabled(false);
}

bool GPSNativeUnicore::_execute(std::string command, Reply reply)
{
    _command = {.text = std::move(command), .expected = reply, .active = true};
    _configurationDetail.clear();
    const auto clearReply = qScopeGuard([this] { _command.active = false; });
    const GPSConfigurationStep step{_command.text, std::chrono::milliseconds(COMMAND_TIMEOUT_MS)};
    const auto wire = _command.text + "\r\n";
    if (!writeCommand(step, {reinterpret_cast<const uint8_t*>(wire.data()), wire.size()})) {
        _configurationDetail =
            QStringLiteral("Unicore command '%1' could not be written").arg(QString::fromStdString(_command.text));
        return false;
    }
    const auto result = awaitCommand([this] { return _command.outcome; });
    _command.active = false;
    if (result.evidence.outcome != GPSCommandOutcome::Acknowledged &&
        result.evidence.outcome != GPSCommandOutcome::ReadbackVerified) {
        if (result.evidence.outcome != GPSCommandOutcome::Cancelled) {
            if (_configurationDetail.isEmpty()) {
                const auto failure = result.evidence.outcome == GPSCommandOutcome::TimedOut
                                         ? QStringLiteral("timed out")
                                     : result.evidence.outcome == GPSCommandOutcome::Rejected
                                         ? QStringLiteral("was rejected or its readback did not match")
                                         : QStringLiteral("failed");
                _configurationDetail =
                    QStringLiteral("Unicore command '%1' %2").arg(QString::fromStdString(_command.text), failure);
            }
            log(GPSProtocolLogLevel::Warning, "Unicore command failed (%d): %s",
                static_cast<int>(result.evidence.outcome), _command.text.c_str());
        }
        return false;
    }
    return true;
}

bool GPSNativeUnicore::_identify(unsigned& baud)
{
    constexpr std::array<unsigned, 8> BAUD_RATES{115200, 230400, 460800, 921600, 57600, 38400, 19200, 9600};
    for (const auto candidate : BAUD_RATES) {
        const unsigned rate = baud ? baud : candidate;
        resetStream();
        _configurationDetail = QStringLiteral("Cannot configure Unicore host serial speed %1").arg(rate);
        if (setBaudrate(static_cast<int>(rate)) == 0 && _execute("VERSIONA", Reply::Version)) {
            baud = rate;
            log(GPSProtocolLogLevel::Debug, "Unicore %s firmware %s", _model.c_str(), _firmware.c_str());
            return true;
        }
        if (baud || ioError() || !_model.empty()) {
            return false;
        }
    }
    return false;
}

int GPSNativeUnicore::configure(unsigned& baud, const GPSConfig& config)
{
    const bool wasBase = _base;
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _lastBaseEpoch.reset();
    _command.active = false;
    _base = true;
    _averaging = !std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode);
    setRTCMEnabled(false);
    resetIOError();
    resetStream();
    _model.clear();
    _firmware.clear();
    _configurationDetail.clear();
    if (wasBase || _base) {
        _publishBase(false, false);
        consume({});
    }

    if (!validateConfiguration(config, {.receiverAveraging = true})) {
        return _configurationFailed(
            QStringLiteral("Invalid Unicore receiver configuration: check the role, base position and survey settings; "
                           "persistent changes are not supported"));
    }
    if (_averaging && !std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)) {
        log(GPSProtocolLogLevel::Warning,
            "Unicore supports receiver-managed averaging, not accuracy-controlled survey");
        return _configurationFailed(
            QStringLiteral("Unicore requires receiver-managed averaging with a duration between 1 and 3600 seconds"));
    }
    _baseConfig = config.base;
    if (!_averaging) {
        _fixedECEF = toEcef(std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position);
    }
    const Operation operation(*this, 45000);
    if (!_identify(baud) || !_execute("UNLOG")) {
        return _configurationFailed();
    }
    // Force a new role transition even when reconnecting to a receiver left in base mode.
    _expectedMode = Mode::Rover;
    if (!_execute("MODE ROVER") || !_execute("MODE", Reply::Mode)) {
        return _configurationFailed();
    }
    for (const auto command : {"GPGGA 1", "GPGST 1", "GPGSV 1", "GPGSA 1"}) {
        if (!_execute(command)) {
            return _configurationFailed();
        }
    }
    std::ostringstream mode;
    mode.imbue(std::locale::classic());
    if (_averaging) {
        // Distance=0 forces newly averaged coordinates; it is NOT an accuracy threshold.
        mode << "MODE BASE TIME "
             << std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDurationSecs << " 0";
        _expectedMode = Mode::AveragingBase;
    } else {
        mode << std::fixed << std::setprecision(4) << "MODE BASE " << _fixedECEF.x << ' ' << _fixedECEF.y << ' '
             << _fixedECEF.z;
        _expectedMode = Mode::FixedBase;
    }
    if (!_execute(mode.str()) || !_execute("MODE", Reply::Mode)) {
        return _configurationFailed();
    }
    _monitorBase = true;
    _publishBase(false, _averaging);
    // Read back before subscribing, so queued periodic output cannot satisfy the query.
    if (!_averaging && !_execute("BESTNAVXYZA", Reply::FixedPosition)) {
        return _configurationFailed();
    }
    if (!_execute("BESTNAVXYZA 1")) {
        return _configurationFailed();
    }
    for (const auto command : {"RTCM1005 1", "RTCM1033 10", "RTCM1074 1", "RTCM1084 1", "RTCM1094 1", "RTCM1124 1"}) {
        if (!_execute(command)) {
            return _configurationFailed();
        }
    }
    _ready = true;
    setRTCMEnabled(_baseValid);
    return 0;
}

int GPSNativeUnicore::_configurationFailed(const QString& reason)
{
    if (!reason.isEmpty()) {
        _configurationDetail = reason;
    }
    if (_monitorBase) {
        _publishBase(false, false);
    }
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    setRTCMEnabled(false);
    consume({});
    if (ioError() != ReadCancelled && ioErrorDetail().isEmpty()) {
        _ioErrorDetail = _configurationDetail;
    }
    return ioError() ? ioError() : -1;
}

void GPSNativeUnicore::_handleVersion(std::string_view body)
{
    std::array<std::string_view, 6> fields{};
    if (NMEA::splitFields(body, fields) != fields.size()) {
        return;
    }
    const auto model = unquote(fields[0]);
    const auto firmware = unquote(fields[1]);
    if (model.empty() || model.size() > 32 || firmware.empty() || firmware.size() > 32) {
        return;
    }
    if (_command.active && _command.expected == Reply::Version && _command.outcome == GPSCommandOutcome::Pending) {
        _model = model;
        _firmware = firmware;
        _command.outcome =
            supportedFirmware(model, firmware) ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
        if (_command.outcome == GPSCommandOutcome::Rejected) {
            _configurationDetail =
                QStringLiteral(
                    "Unsupported Unicore receiver '%1' firmware '%2'; requires UM980 R4.10Build7923+ "
                    "or UM982 R4.10Build7650+")
                    .arg(QString::fromStdString(_model), QString::fromStdString(_firmware));
        }
    } else if (_ready) {
        // An unsolicited identity report can indicate a reboot; never retain old base validity.
        _invalidateBase();
    }
}

void GPSNativeUnicore::_handleMode(std::string_view body)
{
    const auto mode = body.substr(0, body.find(','));
    const auto starts = [&](std::string_view name) {
        return mode == name || (mode.starts_with(name) && mode.size() > name.size() && mode[name.size()] == ' ');
    };
    const bool matches = _expectedMode == Mode::Rover           ? starts("MODE ROVER")
                         : _expectedMode == Mode::AveragingBase ? starts("MODE BASE TIME")
                                                                : mode == "MODE BASE";
    if (_command.active && _command.expected == Reply::Mode && _command.outcome == GPSCommandOutcome::Pending) {
        _command.outcome = matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    } else if (_ready && !matches) {
        _invalidateBase();
    }
}

void GPSNativeUnicore::_handlePosition(std::string_view body)
{
    if (!_monitorBase) {
        return;
    }
    std::array<std::string_view, 28> fields{};
    if (NMEA::splitFields(body, fields) != fields.size()) {
        return;
    }
    const auto x = NMEA::number<double>(fields[2]);
    const auto y = NMEA::number<double>(fields[3]);
    const auto z = NMEA::number<double>(fields[4]);
    if (!x || !y || !z) {
        return;
    }
    const EcefMeters coordinates{*x, *y, *z};
    const auto samePosition = [](const EcefMeters& left, const EcefMeters& right) {
        // ECEF command/readback values have four decimal places; this is not survey accuracy.
        return std::hypot(left.x - right.x, left.y - right.y, left.z - right.z) < 0.02;
    };
    const auto radius = std::hypot(*x, *y, *z);
    const bool fixed = fields[0] == "SOL_COMPUTED" && fields[1] == "FIXEDPOS" && radius > 6000000 && radius < 7000000;
    const bool matches = _averaging || samePosition(coordinates, _fixedECEF);
    if (_command.active && _command.expected == Reply::FixedPosition && fixed &&
        _command.outcome == GPSCommandOutcome::Pending) {
        _command.outcome = matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected;
    }
    if (_ready && _baseValid && (!fixed || !matches || !samePosition(coordinates, _baseECEF))) {
        _invalidateBase();
        return;
    }
    _lastBaseStatus = nowUs();
    _baseValid = fixed && matches;
    if (_baseValid) {
        _baseECEF = coordinates;
    }
    setRTCMEnabled(_ready && _baseValid);
    _publishBase(_baseValid, _averaging && !_baseValid);
}

void GPSNativeUnicore::_publishBase(bool valid, bool active)
{
    GPSNativeSurveyReport report{};
    report.survey.valid = valid;
    report.survey.active = active;
    if (valid) {
        const auto position = fromEcef(_baseECEF);
        report.survey.position = position;
    }
    // BESTNAV's instantaneous sigmas and BASEPOS monitoring are not averaging accuracy or elapsed time.
    surveyInStatus(report);
}

void GPSNativeUnicore::_invalidateBase()
{
    _baseValid = false;
    _monitorBase = false;
    _ready = false;
    setRTCMEnabled(false);
    if (_base) {
        _publishBase(false, false);
    }
    controlFailed();
}

void GPSNativeUnicore::_expireBase()
{
    if (_ready && (ioError() || (_baseValid && nowUs() - _lastBaseStatus > BASE_STATUS_TIMEOUT_US))) {
        _invalidateBase();
    }
}

int GPSNativeUnicore::decodeByte(uint8_t byte)
{
    _expireBase();
    return GPSAsciiProtocol::decodeByte(byte);
}

void GPSNativeUnicore::flushDecoded()
{
    _expireBase();
    GPSAsciiProtocol::flushDecoded();
}

void GPSNativeUnicore::servicePendingCommands()
{
    _expireBase();
}

int GPSNativeUnicore::handleReceiverLine(std::string_view line)
{
    if (line.starts_with("$command,")) {
        if (!_command.active || !validChecksum(line, false)) {
            return 0;
        }
        const auto response = line.find(",response: ", 9);
        if (response == std::string_view::npos || !equalCommand(line.substr(9, response - 9), _command.text)) {
            return 0;
        }
        const auto status = line.substr(response + 11, line.find('*') - response - 11);
        if (_command.outcome == GPSCommandOutcome::Pending) {
            if (status != "OK") {
                _command.outcome = GPSCommandOutcome::Rejected;
            } else if (_command.expected == Reply::Acknowledgment) {
                _command.outcome = GPSCommandOutcome::Acknowledged;
            }
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (!line.starts_with('#')) {
        return 0;
    }
    const auto comma = line.find(',');
    const auto name = line.substr(1, comma - 1);
    if (name != "VERSIONA" && name != "MODE" && name != "BESTNAVXYZA") {
        return 0;
    }
    if (!validChecksum(line, name != "MODE")) {
        return 0;
    }
    const auto semicolon = line.find(';');
    if (semicolon == std::string_view::npos || semicolon >= line.find('*')) {
        return 0;
    }
    std::array<std::string_view, 10> header{};
    if (NMEA::splitFields(line.substr(1, semicolon - 1), header) != header.size()) {
        return 0;
    }
    const auto body = line.substr(semicolon + 1, line.find('*') - semicolon - 1);
    if (name == "VERSIONA") {
        _handleVersion(body);
    } else if (name == "MODE") {
        _handleMode(body);
    } else {
        if (!_monitorBase) {
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        const auto week = NMEA::number<uint16_t>(header[4]);
        const auto milliseconds = NMEA::number<uint32_t>(header[5]);
        if (header[2] != "GPS" || header[3] != "FINE" || !week || !milliseconds || *milliseconds >= 604800000) {
            return 0;
        }
        const uint64_t epoch = uint64_t(*week) * 604800000 + *milliseconds;
        if (_lastBaseEpoch && epoch < *_lastBaseEpoch) {
            _invalidateBase();
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        if (_lastBaseEpoch && epoch == *_lastBaseEpoch) {
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        _lastBaseEpoch = epoch;
        _handlePosition(body);
    }
    return GPSDecodedBatch::PROTOCOL_ACTIVITY;
}

std::string GPSNativeUnicore::receiverIdentity() const
{
    return _model.empty() || _firmware.empty() ? _model + _firmware : _model + ' ' + _firmware;
}
