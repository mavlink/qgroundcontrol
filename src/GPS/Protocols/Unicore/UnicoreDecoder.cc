#include <algorithm>
#include <cmath>

#include "CRC32.h"
#include "UnicoreProtocol.h"

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

void UnicoreProtocol::_handleVersion(std::string_view body)
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
    if (_awaitingReply(Reply::Version)) {
        _model = model;
        _firmware = firmware;
        const bool supported = supportedFirmware(model, firmware);
        resolveReply(supported ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
        if (!supported) {
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

void UnicoreProtocol::_handleMode(std::string_view body)
{
    const auto mode = body.substr(0, body.find(','));
    const auto starts = [&](std::string_view name) {
        return mode == name || (mode.starts_with(name) && mode.size() > name.size() && mode[name.size()] == ' ');
    };
    const bool matches = _expectedMode == Mode::Rover           ? starts("MODE ROVER")
                         : _expectedMode == Mode::AveragingBase ? starts("MODE BASE TIME")
                                                                : mode == "MODE BASE";
    if (_awaitingReply(Reply::Mode)) {
        resolveReply(matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
    } else if (_ready && !matches) {
        _invalidateBase();
    }
}

void UnicoreProtocol::_handlePosition(std::string_view body)
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
    if (fixed && _awaitingReply(Reply::FixedPosition)) {
        resolveReply(matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
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

void UnicoreProtocol::_publishBase(bool valid, bool active)
{
    GPSDecodedSurvey report{};
    report.survey.valid = valid;
    report.survey.active = active;
    if (valid) {
        const auto position = fromEcef(_baseECEF);
        report.survey.position = position;
    }
    // BESTNAV's instantaneous sigmas and BASEPOS monitoring are not averaging accuracy or elapsed time.
    publishSurvey(report);
}

void UnicoreProtocol::_invalidateBase()
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

void UnicoreProtocol::_expireBase()
{
    if (_ready && (hasIOError() || (_baseValid && nowUs() - _lastBaseStatus > BASE_STATUS_TIMEOUT_US))) {
        _invalidateBase();
    }
}

int UnicoreProtocol::decodeByte(uint8_t byte)
{
    _expireBase();
    return GPSAsciiProtocol::decodeByte(byte);
}

void UnicoreProtocol::flushDecoded()
{
    _expireBase();
    GPSAsciiProtocol::flushDecoded();
}

void UnicoreProtocol::servicePendingCommands()
{
    _expireBase();
}

int UnicoreProtocol::handleReceiverLine(std::string_view line)
{
    if (line.starts_with("$command,")) {
        if (!replyPending() || !validChecksum(line, false)) {
            return 0;
        }
        const auto response = line.find(",response: ", 9);
        if (response == std::string_view::npos || !equalCommand(line.substr(9, response - 9), _command.text)) {
            return 0;
        }
        const auto status = line.substr(response + 11, line.find('*') - response - 11);
        if (status != "OK") {
            resolveReply(GPSCommandOutcome::Rejected);
        } else if (_command.expected == Reply::Acknowledgment) {
            resolveReply(GPSCommandOutcome::Acknowledged);
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
