#include "UBXReceiverModel.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <utility>

#include "Protocols/Support/ProtocolTestPackets.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"

namespace GPSTest {

namespace {
constexpr uint8_t CFG_CLASS = 0x06;
constexpr uint8_t CFG_RST = 0x04;
constexpr uint8_t CFG_CFG = 0x09;
constexpr uint8_t PORT_UART1 = 1;
constexpr uint8_t PORT_USB = 3;
constexpr uint8_t CFG_TMODE3 = 0x71;
constexpr uint8_t CFG_VALSET = 0x8a;
constexpr uint8_t CFG_VALGET = 0x8b;
constexpr uint32_t TMODE_MODE = 0x20030001;
constexpr uint32_t TMODE_FIXED_POS_ACC = 0x4003000f;
constexpr uint32_t TMODE_SVIN_MIN_DUR = 0x40030010;
constexpr uint32_t NAVSPG_DYNMODEL = 0x20110021;
constexpr uint32_t SEC_JAMDET_SENSITIVITY_HI = 0x10f60051;
constexpr uint32_t RATE_MEAS = 0x30210001;

QByteArray padded(const QByteArray& text, qsizetype size)
{
    QByteArray result = text;
    result.resize(size, '\0');
    return result;
}

/// A UBX configuration value, whose width its key encodes.
/// The model keeps configuration values as 32 bits; an 8-byte value keeps its low half.
uint32_t configValue(const QByteArray& bytes, qsizetype offset, qsizetype width)
{
    if (offset < 0 || width < 0 || offset + width > bytes.size()) {
        return 0;
    }
    uint64_t value = 0;
    for (qsizetype i = 0; i < width; ++i) {
        value |= uint64_t(static_cast<uint8_t>(bytes[offset + i])) << (8 * i);
    }
    return static_cast<uint32_t>(value);
}

void appendConfigValue(QByteArray& bytes, uint32_t value, unsigned width)
{
    const uint64_t wide = value;
    for (unsigned i = 0; i < width; ++i) {
        bytes.append(static_cast<char>(wide >> (8 * i)));
    }
}
}  // namespace

UBXReceiverModel::UBXReceiverModel(GPSTestClock& clock)
    : _clock(&clock)
{}

UBXReceiverModel::UBXReceiverModel(Receiver model, GPSTestClock& clock)
    : _clock(&clock)
{
    QByteArray hardwareVersion = "00080000";
    QByteArray firmware = "HPG 1.40";
    QByteArray moduleName = "NEO-M8P-2";
    bool modern = false;
    switch (model) {
        case Receiver::M8PBase:
            break;
        case Receiver::F9P:
            hardwareVersion = "00190000";
            firmware = "HPG 1.32";
            moduleName = "ZED-F9P";
            modern = true;
            break;
        case Receiver::M8N:
            firmware = "SPG 3.01";
            moduleName = "NEO-M8N";
            break;
        case Receiver::M9N:
            hardwareVersion = "00190000";
            firmware = "SPG 4.04";
            moduleName = "NEO-M9N";
            modern = true;
            break;
        case Receiver::M10:
            hardwareVersion = "000A0000";
            firmware = "SPG 5.10";
            moduleName = "MAX-M10S";
            modern = true;
            break;
        case Receiver::M8PRover:
            moduleName = "NEO-M8P-0";
            break;
        case Receiver::F9R:
            hardwareVersion = "00190000";
            firmware = "HPS 1.30";
            moduleName = "ZED-F9R";
            modern = true;
            break;
        case Receiver::Unidentified:
            firmware = "UNKNOWN";
            moduleName = "UNKNOWN";
            break;
        case Receiver::U6:
            hardwareVersion = "00040007";
            break;
        case Receiver::M8NEarly:
            break;
    }
    legacy = !modern;
    module = moduleName.toStdString();
    _presetIdentity = padded("EXT CORE", 30) + padded(hardwareVersion, 10) + padded("FWVER=" + firmware, 30) +
                      padded(modern ? "PROTVER=27.31" : "PROTVER=20.30", 30) + padded("MOD=" + moduleName, 30);
    if (model == Receiver::U6) {
        _presetIdentity.truncate(40);
    } else if (model == Receiver::M8NEarly) {
        _presetIdentity = _presetIdentity.first(40) + padded("PROTVER=15.00", 30);
    }
}

void UBXReceiverModel::queueFrame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload)
{
    if (_receiver) {
        _receiver->queueReply(_frame(messageClass, messageId, payload));
    }
}

void UBXReceiverModel::queueFrame(uint16_t message, std::span<const uint8_t> payload)
{
    queueBytes(_frame(message, payload));
}

void UBXReceiverModel::queueBytes(const QByteArray& bytes)
{
    if (_receiver) {
        _receiver->queueReply(bytes);
    }
}

void UBXReceiverModel::queueBytes(std::span<const uint8_t> bytes)
{
    queueBytes(QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size())));
}

void UBXReceiverModel::queueSurveyReply(SurveyReply reply)
{
    if (!_receiver || reply == SurveyReply::Silent) {
        return;
    }

    QByteArray payload(40, '\0');
    payload[36] = static_cast<char>(reply == SurveyReply::Valid);
    payload[37] = static_cast<char>(reply == SurveyReply::Active);
    if (reply == SurveyReply::BadLength) {
        payload.append('\0');
    }

    QByteArray bytes = _frame(UBX::Msg::NAV_SVIN.value(),
                              std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.constData()),
                                                       static_cast<size_t>(payload.size())));
    if (reply == SurveyReply::BadChecksum) {
        bytes.back() ^= 0xff;
    }
    _receiver->queueReply(bytes);
}

std::optional<GPSWriteResult> UBXReceiverModel::interceptWrite(const QByteArray& bytes)
{
    ++transportOperations;
    const uint32_t message = LittleEndian::read<uint16_t>(bytesOf(bytes), 2).value_or(0);
    if (failValsetKey != 0 && message == UBX::Msg::CFG_VALSET.value()) {
        // Key/value entries follow the frame header and the 4-byte VALSET header; the checksum ends the frame.
        qsizetype offset = 10;
        while (offset + 4 <= bytes.size() - 2) {
            const uint32_t key = LittleEndian::read<uint32_t>(bytesOf(bytes), offset).value_or(0);
            offset += 4;
            if (key == failValsetKey) {
                // The link takes part of the payload, and writes less of it.
                return GPSWriteResult{valsetWriteFailure, 9, 7};
            }
            const unsigned storage = key >> 28;
            const qsizetype width = storage <= 2 ? 1 : qsizetype{1} << (storage - 2);
            offset += width;
        }
    }
    return std::nullopt;
}

void UBXReceiverModel::reset(ScriptedReceiver& receiver)
{
    _receiver = &receiver;
    _delayed.clear();
    receiver.setFixedBaudrate(115200);
    receiver.setFatalError(_readError);
    receiver.setReadHandler(
        [this, &receiver](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
            if (!receiver.isCancelled() && !_readError) {
                return std::nullopt;
            }
            return GPSReadResult{receiver.isCancelled() ? GPSReadStatus::Cancelled : GPSReadStatus::Error};
        });
}

void UBXReceiverModel::onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline)
{
    if (!_delayed.empty() && _delayed.front().atUs <= deadline.untilUs) {
        _clock->advanceTo(_delayed.front().atUs);
        _deliverDelayed(receiver);
        return;
    }
    _clock->advanceTo(deadline.untilUs + 1);
}

int UBXReceiverModel::readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const
{
    Q_UNUSED(receiver)
    return std::min({requested, available, coalesceReplies ? requested : static_cast<int>(readChunk)});
}

bool UBXReceiverModel::coalesceReads(const ScriptedReceiver& receiver) const
{
    Q_UNUSED(receiver)
    return coalesceReplies;
}

void UBXReceiverModel::_queueResponse(ScriptedReceiver& receiver, const Response& response,
                                      std::chrono::microseconds delay)
{
    if (_heldReply) {
        const Response held = *std::exchange(_heldReply, std::nullopt);
        _queueResponse(receiver, held);
    }
    delay = std::max(delay, replyDelay);
    if (delay > std::chrono::microseconds::zero() || !_delayed.empty()) {
        const uint64_t due = _clock->nowUs() + static_cast<uint64_t>(delay.count());
        _delayed.push_back({_delayed.empty() ? due : std::max(due, _delayed.back().atUs), response});
        return;
    }
    receiver.queueReply(response.bytes);
}

void UBXReceiverModel::_deliverDelayed(ScriptedReceiver& receiver)
{
    while (!_delayed.empty() && _delayed.front().atUs <= _clock->nowUs()) {
        receiver.queueReply(_delayed.front().response.bytes);
        _delayed.pop_front();
    }
}

QByteArray UBXReceiverModel::_frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload)
{
    const auto frame =
        ubxFrame(uint16_t(messageClass) | (uint16_t(messageId) << 8),
                 {reinterpret_cast<const uint8_t*>(payload.constData()), static_cast<size_t>(payload.size())});
    return QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size()));
}

QByteArray UBXReceiverModel::_frame(uint16_t message, std::span<const uint8_t> payload)
{
    const auto frame = ubxFrame(message, payload);
    return QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size()));
}

QByteArray UBXReceiverModel::_ackFrame(uint16_t message, bool accepted)
{
    const std::array<uint8_t, 2> payload{uint8_t(message & 0xff), uint8_t(message >> 8)};
    return _frame(accepted ? UBX::Msg::ACK_ACK.value() : UBX::Msg::ACK_NAK.value(), payload);
}

QByteArray UBXReceiverModel::_identityPayload()
{
    if (!_presetIdentity.isEmpty()) {
        return _presetIdentity;
    }
    QByteArray version(70, '\0');
    std::memcpy(version.data(), "HPG 1.32", 8);
    const std::string hardwareVersion = hardware.empty() ? (legacy                 ? "00080000"
                                                            : module == "ZED-X20P" ? "000B0000"
                                                                                   : "00190000")
                                                         : hardware;
    if (hardwareVersion.size() == 8) {
        std::memcpy(version.data() + 30, hardwareVersion.data(), 8);
    } else {
        wireValid = false;
    }
    const std::string identity = "MOD=" + module;
    if (identity.size() < 30) {
        std::memcpy(version.data() + 40, identity.c_str(), identity.size());
    } else {
        wireValid = false;
    }
    if (!protocol.empty()) {
        const std::string extension = "PROTVER=" + protocol;
        if (extension.size() < 30) {
            version.resize(100, '\0');
            std::copy(extension.begin(), extension.end(), version.begin() + 70);
        } else {
            wireValid = false;
        }
    }
    return version;
}

void UBXReceiverModel::_queueAck(ScriptedReceiver& receiver, uint8_t messageId, bool accepted,
                                 std::chrono::microseconds delay)
{
    Response response{_ackFrame(uint16_t(CFG_CLASS | (messageId << 8)), accepted)};
    if (messageId == CFG_VALSET && _delayNextValsetAck) {
        _heldReply = response;
        _delayNextValsetAck = false;
        ++optionalAckDelays;
    } else {
        _queueResponse(receiver, response, delay);
    }
}

std::optional<QByteArray> UBXReceiverModel::takeCommand(QByteArray& pending)
{
    if (pending.size() < 6) {
        return std::nullopt;
    }
    const auto payloadSize = LittleEndian::read<uint16_t>(bytesOf(pending), 4).value_or(0);
    const auto frameSize = payloadSize + 8;
    if (pending.size() < frameSize) {
        return std::nullopt;
    }
    const QByteArray frame = pending.first(frameSize);
    pending.remove(0, frameSize);
    return frame;
}

GPSWriteResult UBXReceiverModel::handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                               const ScriptedReceiver::WriteContext& context)
{
    Q_UNUSED(context)
    if (!_handleFrame(receiver, command)) {
        return {GPSWriteStatus::Error};
    }
    const int length = command.size();
    return {GPSWriteStatus::Completed, length, length};
}

std::optional<bool> UBXReceiverModel::handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate)
{
    Q_UNUSED(receiver)
    hostBaud = baudrate;
    ++transportOperations;
    return std::nullopt;
}

bool UBXReceiverModel::_handleFrame(ScriptedReceiver& receiver, const QByteArray& frame)
{
    if (frame.size() < 8) {
        wireValid = false;
        return false;
    }

    const auto message = LittleEndian::read<uint16_t>(bytesOf(frame), 2).value_or(0);
    const QByteArray payload = frame.sliced(6, frame.size() - 8);
    if (frame != _frame(message, bytesOf(payload))) {
        wireValid = false;
        return false;
    }

    const bool configuration = (message & 0xff) == UBX::MsgClass::CFG;
    if (message == UBX::Msg::MON_VER.value()) {
        identityBauds.push_back(hostBaud);
    }
    if (configuration && message != UBX::Msg::CFG_VALGET.value() &&
        !(message == UBX::Msg::CFG_TMODE3.value() && payload.isEmpty()) &&
        !(message == UBX::Msg::CFG_MSG.value() && payload.size() == 2)) {
        ++configurationWrites;
    }
    if (configuration && ((message >> 8) == CFG_RST || (message >> 8) == CFG_CFG)) {
        ++resetCommands;
    }
    if (receiverBaud != 0 && !usb && hostBaud != receiverBaud) {
        return true;
    }

    if (message == UBX::Msg::CFG_TMODE3.value() && payload.isEmpty()) {
        QByteArray response(40, '\0');
        response[2] = static_cast<char>(timeMode);
        return _replyToReadback(receiver, CFG_TMODE3, response, TMODE_MODE);
    }
    if (message == UBX::Msg::CFG_VALGET.value()) {
        if (payload.size() < 8 || payload.size() > 40 || ((payload.size() - 4) % 4) != 0 ||
            payload.first(4) != QByteArray(4, '\0')) {
            wireValid = false;
            return false;
        }
        QByteArray response = QByteArray::fromHex("01000000");
        for (qsizetype offset = 4; offset < payload.size(); offset += 4) {
            const uint32_t key = LittleEndian::read<uint32_t>(bytesOf(payload), offset).value_or(0);
            if (std::ranges::find(unsupportedKeys, key) != unsupportedKeys.end() ||
                (delayOptionalNak && key == SEC_JAMDET_SENSITIVITY_HI)) {
                _queueAck(receiver, CFG_VALGET, false);
                return true;
            }
            uint32_t value = 0;
            if (key == TMODE_MODE) {
                value = timeMode;
            } else if (const auto found = currentSettings.find(key); found != currentSettings.end()) {
                value = found->second;
            }
            response.append(QByteArray(sizeof(key), '\0'));
            (void) LittleEndian::write(mutableBytesOf(response), response.size() - sizeof(key), key);
            const unsigned code = key >> 28;
            appendConfigValue(response, value, code <= 2 ? 1 : 1u << (code - 2));
        }
        return _replyToReadback(receiver, CFG_VALGET, response,
                                LittleEndian::read<uint32_t>(bytesOf(payload), 4).value_or(0));
    }

    if (message == UBX::Msg::MON_VER.value()) {
        if (!payload.isEmpty()) {
            wireValid = false;
            return false;
        }
        const QByteArray identity = _identityPayload();
        QByteArray response = _frame(message, bytesOf(identity));
        if (corruptIdentity) {
            response.back() ^= 0x01;
        }
        if (!corruptVersionReplies) {
            _queueResponse(receiver, Response{response});
            return true;
        }
        // A version without base support, with a bad checksum, on both sides of the valid one.
        QByteArray nonBase = identity;
        nonBase.replace(40, 30, padded("FWVER=SPG 3.01", 30));
        QByteArray corrupt = _frame(message, bytesOf(nonBase));
        corrupt.back() ^= 0x01;
        _queueResponse(receiver, Response{corrupt});
        _queueResponse(receiver, Response{response});
        _queueResponse(receiver, Response{corrupt});
        return true;
    }

    if (message == UBX::Msg::NAV_SVIN.value()) {
        if (!payload.isEmpty() || modes.empty() || modes.back() != 0) {
            wireValid = false;
            return false;
        }
        ++surveyPolls;
        if (!surveyReplies.isEmpty()) {
            queueSurveyReply(surveyReplies.at(std::min<qsizetype>(surveyPolls - 1, surveyReplies.size() - 1)));
            return true;
        }
        QByteArray status(40, '\0');
        status[37] = static_cast<char>(timeMode == 1 || surveyStopStuck);
        (void) LittleEndian::write<uint32_t>(mutableBytesOf(status), 8, retainedSurveyDuration);
        _queueResponse(receiver, Response{_frame(message, bytesOf(status))});
        return true;
    }

    if (legacy) {
        return _handleLegacyFrame(receiver, message, payload);
    }
    if (message != UBX::Msg::CFG_VALSET.value() || payload.size() < 4) {
        wireValid = false;
        return false;
    }
    return _handleValset(receiver, message, payload);
}

bool UBXReceiverModel::_handleValset(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload)
{
    if (silencePortConfiguration) {
        return true;
    }

    std::map<uint32_t, uint32_t> settings;
    for (qsizetype offset = 4; offset < payload.size();) {
        const uint32_t key = LittleEndian::read<uint32_t>(bytesOf(payload), offset).value_or(0);
        if ((key & 0xffff0000u) == 0x10710000u || (key & 0xffff0000u) == 0x10720000u) {
            wireValid = false;
            return false;
        }
        offset += 4;
        const unsigned sizeCode = (key >> 28) & 7;
        if (sizeCode < 1 || sizeCode > 5) {
            wireValid = false;
            return false;
        }
        const qsizetype width = sizeCode <= 2 ? 1 : qsizetype{1} << (sizeCode - 2);
        if (offset + width > payload.size()) {
            wireValid = false;
            return false;
        }
        settings[key] = configValue(payload, offset, width);
        offset += width;
    }

    if (!_heldBaudAck.isEmpty()) {
        queueBytes(_heldBaudAck);
        _heldBaudAck.clear();
        if (rejectAfterBaudChange) {
            return true;
        }
        if (nakAfterBaudChange) {
            _queueResponse(receiver, Response{_ackFrame(message, false)});
        }
    }

    const bool unsupported = std::ranges::any_of(settings, [this](const auto& setting) {
        return std::ranges::find(unsupportedKeys, setting.first) != unsupportedKeys.end();
    });
    if (unsupported && loseUnsupportedNak) {
        return true;
    }
    bool reject = unsupported;
    if (settings.contains(SEC_JAMDET_SENSITIVITY_HI) && delayOptionalAck) {
        _delayNextValsetAck = true;
        reject = reject || delayOptionalNak;
    }
    if (const auto rate = settings.find(RATE_MEAS);
        rejectRtcmActivation && rate != settings.end() && rate->second == 1000) {
        reject = true;
    }
    if (const auto duration = settings.find(TMODE_SVIN_MIN_DUR); duration != settings.end()) {
        surveyDuration = duration->second;
    }
    if (const auto accuracy = settings.find(TMODE_FIXED_POS_ACC); accuracy != settings.end()) {
        fixedAccuracy = accuracy->second;
    }
    if (const auto model = settings.find(NAVSPG_DYNMODEL); model != settings.end()) {
        navigationModel = model->second;
    }
    const auto apply = [this, &settings] {
        for (const auto& setting : settings) {
            currentSettings[setting.first] = setting.second;
        }
    };
    const auto mode = settings.find(UBX::Cfg::TMODE_MODE.id);
    if (mode != settings.end()) {
        modes.push_back(mode->second);
        if (mode->second == 0) {
            disabledAt = _clock->nowUs();
            if (reject) {
                _queueAck(receiver, CFG_VALSET, false);
                return true;
            }
            return _disableTimeMode(receiver, CFG_VALSET, apply);
        }
        if (mode->second == 1) {
            ++starts;
            startedAt = _clock->nowUs();
            startSettings = settings;
            reject = reject || rejectStart;
        }
    }
    if (const auto messageRate = settings.find(UBX::Cfg::MSGOUT_RTCM_3X_TYPE1005.port(UBX::MsgOutPort::UART1).id);
        messageRate != settings.end() && messageRate->second == 1) {
        ++rtcmEnables;
    }
    if (module == "NEO-M9N" &&
        (settings.contains(UBX::Cfg::UART1OUTPROT_RTCM3X.id) || settings.contains(UBX::Cfg::USBOUTPROT_RTCM3X.id))) {
        reject = true;
    }
    if (!reject) {
        apply();
        if (mode != settings.end()) {
            timeMode = mode->second;
        }
    }
    if (const auto rate = settings.find(UBX::Cfg::UART1_BAUDRATE.id);
        rate != settings.end() && receiverBaud != 0 && rate->second != receiverBaud) {
        if (!ignoreBaudChange) {
            receiverBaud = rate->second;
        } else {
            currentSettings[rate->first] = receiverBaud;
        }
        if (loseBaudAck) {
            _heldBaudAck = _ackFrame(message, true);
            return true;
        }
    }

    _queueAck(receiver, CFG_VALSET, !reject);
    return true;
}

bool UBXReceiverModel::_handleLegacyFrame(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload)
{
    bool unsupported = false;
    RateAck rateFault = RateAck::OnTime;
    if (message == UBX::Msg::CFG_NAV5.value()) {
        if (payload.size() <= 2) {
            wireValid = false;
            return false;
        }
        navigationModel = static_cast<uint8_t>(payload[2]);
    } else if (message == UBX::Msg::CFG_PRT.value()) {
        if (payload.size() != 40 || static_cast<uint8_t>(payload[0]) != PORT_UART1 ||
            static_cast<uint8_t>(payload[20]) != PORT_USB) {
            wireValid = false;
            return false;
        }
        const auto rate = LittleEndian::read<uint32_t>(bytesOf(payload), 8).value_or(0);
        if (rate != LittleEndian::read<uint32_t>(bytesOf(payload), 28).value_or(0)) {
            wireValid = false;
            return false;
        }
        if (receiverBaud != 0 && rate != receiverBaud) {
            receiverBaud = rate;
            if (loseBaudAck) {
                return true;
            }
        }
    } else if (message == UBX::Msg::CFG_MSG.value()) {
        if (payload.size() == 2) {
            return _replyToRatePoll(receiver, LittleEndian::read<uint16_t>(bytesOf(payload), 0).value_or(0));
        }
        if (payload.size() != 3) {
            wireValid = false;
            return false;
        }
        const auto output = LittleEndian::read<uint16_t>(bytesOf(payload), 0).value_or(0);
        if (output == rateAckMessage) {
            rateFault = rateAck;
        }
        if (rateFault == RateAck::Ignored) {
            return true;
        }
        unsupported = std::ranges::find(unsupportedMessages, output) != unsupportedMessages.end() ||
                      (rejectRtcmActivation && (output & 0xff) == UBX::MsgClass::RTCM3 && payload[2] != 0);
        if (!unsupported) {
            messageRates[output] = static_cast<uint8_t>(payload[2]);
        }
    } else if (message == UBX::Msg::CFG_TMODE3.value()) {
        const uint32_t mode = LittleEndian::read<uint16_t>(bytesOf(payload), 2).value_or(0) & 0xff;
        modes.push_back(mode);
        fixedAccuracy = LittleEndian::read<uint32_t>(bytesOf(payload), 20).value_or(0);
        surveyDuration = LittleEndian::read<uint32_t>(bytesOf(payload), 24).value_or(0);
        if (mode == 0) {
            disabledAt = _clock->nowUs();
            return _disableTimeMode(receiver, CFG_TMODE3, [] {});
        }
        timeMode = mode;
    }

    const bool reject = message == UBX::Msg::CFG_VALSET.value() || unsupported ||
                        (rejectRtcmActivation && message == UBX::Msg::CFG_RATE.value() &&
                         LittleEndian::read<uint16_t>(bytesOf(payload), 0).value_or(0) == 1000);
    if (!unsupported && message == UBX::Msg::CFG_MSG.value() &&
        LittleEndian::read<uint16_t>(bytesOf(payload), 0).value_or(0) == UBX::Msg::RTCM3_1005.value() &&
        static_cast<uint8_t>(payload[2]) > 0) {
        ++rtcmEnables;
    }

    const Response ack{_ackFrame(message, !reject)};
    if (rateFault == RateAck::Late) {
        _heldReply = ack;
    } else if (rateFault != RateAck::Lost) {
        _queueResponse(receiver, ack);
    }
    return true;
}

bool UBXReceiverModel::_disableTimeMode(ScriptedReceiver& receiver, uint8_t messageId,
                                        const std::function<void()>& apply)
{
    if (staleDisableAck) {
        _queueAck(receiver, messageId, true);
    }
    if (disableReply == DisableReply::Ack || disableReply == DisableReply::Timeout ||
        disableReply == DisableReply::WrongAck || disableReply == DisableReply::CorruptAck) {
        timeMode = 0;
        if (!surveyStopStuck) {
            retainedSurveyDuration = 0;
        }
        apply();
    }
    return _replyToSetting(receiver, messageId, disableReply);
}

bool UBXReceiverModel::_replyToRatePoll(ScriptedReceiver& receiver, uint16_t output)
{
    ++ratePolls;
    if (silentRatePoll) {
        return true;
    }
    const bool unsupported = std::ranges::find(unsupportedMessages, output) != unsupportedMessages.end();
    if (!unsupported) {
        // The rate a CFG-MSG sets applies to the port the command arrived on.
        QByteArray rates(8, '\0');
        rates[0] = static_cast<char>(output & 0xff);
        rates[1] = static_cast<char>(output >> 8);
        const auto rate = messageRates.find(output);
        rates[2 + (usb ? PORT_USB : PORT_UART1)] = static_cast<char>(rate == messageRates.end() ? 0 : rate->second);
        _queueResponse(receiver, Response{_frame(CFG_CLASS, 0x01, rates)});
    }
    _queueAck(receiver, 0x01, !unsupported, ratePollAckDelay);
    return true;
}

bool UBXReceiverModel::_replyToSetting(ScriptedReceiver& receiver, uint8_t messageId, DisableReply reply)
{
    if (reply == DisableReply::Cancelled) {
        receiver.cancel();
        return true;
    }

    if (reply == DisableReply::Nak) {
        _queueAck(receiver, messageId, false);
        return true;
    }
    if (reply == DisableReply::Timeout) {
        return true;
    }
    if (reply == DisableReply::WrongAck) {
        _queueAck(receiver, 0x01, true);
        return true;
    }
    if (reply == DisableReply::CorruptAck) {
        Response response{_ackFrame(uint16_t(CFG_CLASS | (messageId << 8)), true)};
        response.bytes.back() ^= 0x01;
        _queueResponse(receiver, response);
    } else {
        _queueAck(receiver, messageId, true);
    }
    return true;
}

bool UBXReceiverModel::_replyToReadback(ScriptedReceiver& receiver, uint8_t messageId, QByteArray payload, quint32 key)
{
    const auto reply = faultReadbackKey == 0 || faultReadbackKey == key ? readbackReply : ReadbackReply::Value;
    switch (reply) {
        case ReadbackReply::Value:
            break;
        case ReadbackReply::Nak:
            _queueAck(receiver, messageId, false);
            return true;
        case ReadbackReply::Timeout:
            return true;
        case ReadbackReply::AckOnly:
            _queueAck(receiver, messageId, true);
            return true;
        case ReadbackReply::WrongMessage:
            messageId = messageId == CFG_VALGET ? CFG_TMODE3 : CFG_VALGET;
            break;
        case ReadbackReply::WrongKey:
            payload[4] ^= 0x01;
            break;
        case ReadbackReply::WrongValue:
            payload[messageId == CFG_VALGET ? 8 : 2] ^= 0x01;
            break;
        case ReadbackReply::WrongLayer:
            payload[1] = 1;
            break;
        case ReadbackReply::WrongPosition:
            payload[2] = 1;
            break;
        case ReadbackReply::WrongVersion:
            payload[0] = 2;
            break;
        case ReadbackReply::Corrupt:
            break;
        case ReadbackReply::Truncated:
            payload.chop(1);
            break;
        case ReadbackReply::DuplicateValue:
            if (messageId == CFG_VALGET && payload.size() >= 9) {
                payload.append(payload.sliced(4, 5));
            }
            break;
        case ReadbackReply::Oversized:
            payload.resize(1024, '\0');
            break;
        case ReadbackReply::WriteError:
            return false;
        case ReadbackReply::ReadError:
            _readError = true;
            receiver.setFatalError(true);
            return true;
    }
    QByteArray response = _frame(CFG_CLASS, messageId, payload);
    if (reply == ReadbackReply::Corrupt) {
        response.back() ^= 0x01;
    }
    _queueResponse(receiver, Response{response});
    return true;
}

}  // namespace GPSTest
