#include "Unicore/UnicoreDecoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <variant>

#include "CRC32.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSFrame.h"
#include "NMEASentence.h"

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

bool modeReadback(Unicore::Plan::Reply reply)
{
    using Unicore::Plan::Reply;
    return reply == Reply::RoverMode || reply == Reply::AveragingMode || reply == Reply::FixedMode;
}

QByteArray bytes(std::string_view text)
{
    return {text.data(), static_cast<qsizetype>(text.size())};
}

}  // namespace

namespace Unicore {

Decoder::Decoder(bool satelliteInfoEnabled)
    : _nmea(GPSNMEAStream::Navigation::StandardNMEA, satelliteInfoEnabled)
{
    _nmea.setRTCMEnabled(false);
}

QString Decoder::identity() const
{
    return QString::fromUtf8(_model.isEmpty() || _firmware.isEmpty() ? _model + _firmware : _model + ' ' + _firmware);
}

void Decoder::startSession(GPSStreamDemux& stream, GPSDecodeContext& context, bool averaging)
{
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _lastBaseEpoch.reset();
    _averaging = averaging;
    _nmea.setRTCMEnabled(false);
    _nmea.reset(stream);
    _model.clear();
    _firmware.clear();
    _rejection.clear();
    _publishBase(context, false, false);
}

void Decoder::expect(const Plan::Command& command)
{
    _command = command;
    _rejection.clear();
    if (modeReadback(command.reply)) {
        _expectedMode = command.reply;
    }
}

void Decoder::monitorBase(GPSDecodeContext& context)
{
    _monitorBase = true;
    _publishBase(context, false, _averaging);
}

void Decoder::finishSession()
{
    _ready = true;
    _nmea.setRTCMEnabled(_baseValid);
}

void Decoder::armDecodeOnly(const GPSBaseStationConfig::Mode& mode)
{
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&mode);
    _averaging = !fixed;
    _expectedMode = fixed ? Plan::Reply::FixedMode : Plan::Reply::AveragingMode;
    if (fixed) {
        _fixedECEF = GPSProtocolMath::toEcef(fixed->position);
    }
    _monitorBase = true;
    _ready = true;
}

void Decoder::failSession(GPSDecodeContext& context)
{
    if (_monitorBase) {
        _publishBase(context, false, false);
    }
    _ready = false;
    _monitorBase = false;
    _baseValid = false;
    _nmea.setRTCMEnabled(false);
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
    _expireBase(context);
    _nmea.flush(context);
}

bool Decoder::_awaiting(GPSDecodeContext& context, Plan::Reply reply) const
{
    return context.replyPending() && _command.reply == reply;
}

GPSReceiveUpdates Decoder::_decodeAcknowledgement(std::string_view line, GPSDecodeContext& context)
{
    if (!context.replyPending() || !validChecksum(line, false)) {
        return {};
    }
    const auto response = line.find(",response: ", 9);
    const std::string_view command(_command.text.constData(), static_cast<size_t>(_command.text.size()));
    if (response == std::string_view::npos || !equalCommand(line.substr(9, response - 9), command)) {
        return {};
    }
    const auto status = line.substr(response + 11, line.find('*') - response - 11);
    if (status != "OK") {
        context.resolveReply(GPSCommandOutcome::Rejected);
    } else if (_command.reply == Plan::Reply::Acknowledgement) {
        context.resolveReply(GPSCommandOutcome::Acknowledged);
    }
    return GPSReceiveUpdate::Activity;
}

GPSReceiveUpdates Decoder::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    if (line.starts_with("$command,")) {
        return _decodeAcknowledgement(line, context);
    }
    if (!line.starts_with('#')) {
        return {};
    }
    const auto comma = line.find(',');
    const auto name = line.substr(1, comma - 1);
    if (name != "VERSIONA" && name != "MODE" && name != "BESTNAVXYZA") {
        return {};
    }
    if (!validChecksum(line, name != "MODE")) {
        return {};
    }
    const auto semicolon = line.find(';');
    if (semicolon == std::string_view::npos || semicolon >= line.find('*')) {
        return {};
    }
    std::array<std::string_view, 10> header{};
    if (NMEA::splitFields(line.substr(1, semicolon - 1), header) != header.size()) {
        return {};
    }
    const auto body = line.substr(semicolon + 1, line.find('*') - semicolon - 1);
    if (name == "VERSIONA") {
        _handleVersion(body, context);
    } else if (name == "MODE") {
        _handleMode(body, context);
    } else {
        if (!_monitorBase) {
            return GPSReceiveUpdate::Activity;
        }
        const auto week = NMEA::number<uint16_t>(header[4]);
        const auto milliseconds = NMEA::number<uint32_t>(header[5]);
        if (header[2] != "GPS" || header[3] != "FINE" || !week || !milliseconds || *milliseconds >= 604800000) {
            return {};
        }
        const uint64_t epoch = uint64_t(*week) * 604800000 + *milliseconds;
        if (_lastBaseEpoch && epoch < *_lastBaseEpoch) {
            _invalidateBase(context);
            return GPSReceiveUpdate::Activity;
        }
        if (_lastBaseEpoch && epoch == *_lastBaseEpoch) {
            return GPSReceiveUpdate::Activity;
        }
        _lastBaseEpoch = epoch;
        _handlePosition(body, context);
    }
    return GPSReceiveUpdate::Activity;
}

void Decoder::_handleVersion(std::string_view body, GPSDecodeContext& context)
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
    if (_awaiting(context, Plan::Reply::Version)) {
        _model = bytes(model);
        _firmware = bytes(firmware);
        const bool supported = supportedFirmware(model, firmware);
        context.resolveReply(supported ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
        if (!supported) {
            _rejection = QStringLiteral(
                             "Unsupported Unicore receiver '%1' firmware '%2'; requires UM980 R4.10Build7923+ "
                             "or UM982 R4.10Build7650+")
                             .arg(QString::fromUtf8(_model), QString::fromUtf8(_firmware));
        }
    } else if (_ready) {
        // An unsolicited identity report can indicate a reboot; never retain old base validity.
        _invalidateBase(context);
    }
}

void Decoder::_handleMode(std::string_view body, GPSDecodeContext& context)
{
    const auto mode = body.substr(0, body.find(','));
    const auto starts = [&](std::string_view name) {
        return mode == name || (mode.starts_with(name) && mode.size() > name.size() && mode[name.size()] == ' ');
    };
    const bool matches = _expectedMode == Plan::Reply::RoverMode       ? starts("MODE ROVER")
                         : _expectedMode == Plan::Reply::AveragingMode ? starts("MODE BASE TIME")
                                                                       : mode == "MODE BASE";
    if (context.replyPending() && modeReadback(_command.reply)) {
        context.resolveReply(matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
    } else if (_ready && !matches) {
        _invalidateBase(context);
    }
}

void Decoder::_handlePosition(std::string_view body, GPSDecodeContext& context)
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
    const GPSProtocolMath::Ecef coordinates{*x, *y, *z};
    const auto samePosition = [](const GPSProtocolMath::Ecef& left, const GPSProtocolMath::Ecef& right) {
        // ECEF command/readback values have four decimal places; this is not survey accuracy.
        return std::hypot(left.x - right.x, left.y - right.y, left.z - right.z) < 0.02;
    };
    const auto radius = std::hypot(*x, *y, *z);
    const bool fixed = fields[0] == "SOL_COMPUTED" && fields[1] == "FIXEDPOS" && radius > 6000000 && radius < 7000000;
    const bool matches = _averaging || samePosition(coordinates, _fixedECEF);
    if (fixed && _awaiting(context, Plan::Reply::FixedPosition)) {
        context.resolveReply(matches ? GPSCommandOutcome::ReadbackVerified : GPSCommandOutcome::Rejected);
    }
    if (_ready && _baseValid && (!fixed || !matches || !samePosition(coordinates, _baseECEF))) {
        _invalidateBase(context);
        return;
    }
    _lastBaseStatus = context.nowUs();
    _baseValid = fixed && matches;
    if (_baseValid) {
        _baseECEF = coordinates;
    }
    _nmea.setRTCMEnabled(_ready && _baseValid);
    _publishBase(context, _baseValid, _averaging && !_baseValid);
}

void Decoder::_publishBase(GPSDecodeContext& context, bool valid, bool active)
{
    GPSDecodedSurvey report{};
    report.survey.valid = valid;
    report.survey.active = active;
    if (valid) {
        report.survey.position = GPSProtocolMath::fromEcef(_baseECEF);
    }
    // BESTNAV's instantaneous sigmas and BASEPOS monitoring are not averaging accuracy or elapsed time.
    context.sink().publishSurvey(report);
}

void Decoder::_invalidateBase(GPSDecodeContext& context)
{
    _baseValid = false;
    _monitorBase = false;
    _ready = false;
    _nmea.setRTCMEnabled(false);
    _publishBase(context, false, false);
    context.failControl();
}

void Decoder::_expireBase(GPSDecodeContext& context)
{
    if (_ready && (context.failed() || (_baseValid && context.nowUs() - _lastBaseStatus > BASE_STATUS_TIMEOUT_US))) {
        _invalidateBase(context);
    }
}

}  // namespace Unicore
