#include "UBXReceiverModel.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

#include <QtCore/QThread>
#include <QtCore/QtEndian>

#include "../../../../src/GPS/Protocols/UBX/Generated/UBXConfigKeys.h"
#include "../../../../src/GPS/Protocols/UBX/Generated/UBXMessageIds.h"
#include "ProtocolTestPackets.h"

namespace {
constexpr uint8_t CFG_CLASS = 0x06;
constexpr uint8_t PORT_UART1 = 1;
constexpr uint8_t PORT_USB = 3;
constexpr uint8_t CFG_TMODE3 = 0x71;
constexpr uint8_t CFG_VALSET = 0x8a;
constexpr uint8_t CFG_VALGET = 0x8b;
constexpr uint32_t TMODE_MODE = 0x20030001;
constexpr uint32_t TMODE_FIXED_POS_ACC = 0x4003000f;
constexpr uint32_t TMODE_SVIN_MIN_DUR = 0x40030010;
constexpr uint32_t TMODE_SVIN_ACC_LIMIT = 0x40030011;
constexpr uint32_t NAVSPG_DYNMODEL = 0x20110021;
constexpr uint32_t SEC_JAMDET_SENSITIVITY_HI = 0x10f60051;

QByteArray padded(const QByteArray& text, qsizetype size)
{
    QByteArray result = text;
    result.resize(size, '\0');
    return result;
}

uint32_t littleEndian(const QByteArray& bytes, qsizetype offset, qsizetype width)
{
    if (offset < 0 || width < 0 || offset + width > bytes.size()) {
        return 0;
    }
    uint32_t value = 0;
    for (qsizetype i = 0; i < width; ++i) {
        value |= uint32_t(static_cast<uint8_t>(bytes[offset + i])) << (8 * i);
    }
    return value;
}

void appendLittleEndian(QByteArray& bytes, uint32_t value, unsigned width)
{
    for (unsigned i = 0; i < width; ++i) {
        bytes.append(static_cast<char>(value >> (8 * i)));
    }
}
}  // namespace

UBXReceiverModel::UBXReceiverModel(Receiver model, GPSTestClock& clock)
    : _clock(&clock)
{
    QByteArray hardwareVersion = "00080000";
    QByteArray firmware = "HPG 1.40";
    QByteArray moduleName = "NEO-M8P-2";
    switch (model) {
        case Receiver::M8PBase:
            break;
        case Receiver::F9P:
            hardwareVersion = "00190000";
            firmware = "HPG 1.32";
            moduleName = "ZED-F9P";
            _modern = true;
            break;
        case Receiver::M8N:
            firmware = "SPG 3.01";
            moduleName = "NEO-M8N";
            _supportsTimeMode = false;
            break;
        case Receiver::M9N:
            hardwareVersion = "00190000";
            firmware = "SPG 4.04";
            moduleName = "NEO-M9N";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Receiver::M10:
            hardwareVersion = "000A0000";
            firmware = "SPG 5.10";
            moduleName = "MAX-M10S";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Receiver::M8PRover:
            moduleName = "NEO-M8P-0";
            _supportsTimeMode = false;
            break;
        case Receiver::F9R:
            hardwareVersion = "00190000";
            firmware = "HPS 1.30";
            moduleName = "ZED-F9R";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Receiver::Unidentified:
            firmware = "UNKNOWN";
            moduleName = "UNKNOWN";
            break;
        case Receiver::U6:
            hardwareVersion = "00040007";
            _supportsTimeMode = false;
            break;
        case Receiver::M8NEarly:
            _supportsTimeMode = false;
            break;
    }
    _version = padded("EXT CORE", 30) + padded(hardwareVersion, 10) + padded("FWVER=" + firmware, 30) +
               padded(_modern ? "PROTVER=27.31" : "PROTVER=20.30", 30) + padded("MOD=" + moduleName, 30);
    if (model == Receiver::U6) {
        _version.truncate(40);
    } else if (model == Receiver::M8NEarly) {
        _version = _version.first(40) + padded("PROTVER=15.00", 30);
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

void UBXReceiverModel::queueBufferWarning(bool validChecksum)
{
    const QByteArray warning = "txbuf alloc";
    QByteArray bytes = _frame(UBX::Msg::INF_ERROR.value(),
                              std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(warning.constData()),
                                                       static_cast<size_t>(warning.size())));
    if (!validChecksum) {
        bytes.back() ^= 0xff;
    }
    queueBytes(bytes);
}

std::optional<GPSWriteResult> UBXReceiverModel::interceptLowLevelWrite(const QByteArray& bytes)
{
    if (!lowLevelProtocolBehavior) {
        return std::nullopt;
    }

    ++transportOperations;
    if (failCommsWrite && _partialWriteProbe.isEmpty() && bytes.size() >= 4 &&
        static_cast<uint8_t>(bytes[2]) == uint8_t(UBX::Msg::MON_COMMS.value()) &&
        static_cast<uint8_t>(bytes[3]) == uint8_t(UBX::Msg::MON_COMMS.value() >> 8)) {
        ++commsPolls;
        return GPSWriteResult{GPSWriteStatus::Unsupported};
    }
    if (failPollWrite && _partialWriteProbe.isEmpty() && bytes.size() >= 4 &&
        static_cast<uint8_t>(bytes[2]) == uint8_t(UBX::Msg::NAV_SVIN.value()) &&
        static_cast<uint8_t>(bytes[3]) == uint8_t(UBX::Msg::NAV_SVIN.value() >> 8)) {
        return GPSWriteResult{GPSWriteStatus::Unsupported};
    }
    if (failValsetKey != 0 && _partialWriteProbe.size() == 6 &&
        littleEndian(_partialWriteProbe, 2, 2) == UBX::Msg::CFG_VALSET.value() && bytes.size() >= 4) {
        qsizetype offset = 4;
        while (offset + 4 <= bytes.size()) {
            const uint32_t key = littleEndian(bytes, offset, 4);
            offset += 4;
            if (key == failValsetKey) {
                _partialWriteProbe.clear();
                return GPSWriteResult{valsetWriteFailure, 3, 1};
            }
            const unsigned storage = key >> 28;
            const qsizetype width = storage <= 2 ? 1 : qsizetype{1} << (storage - 2);
            offset += width;
        }
    }

    _partialWriteProbe += bytes;
    if (_partialWriteProbe.size() >= 6) {
        const qsizetype frameSize = littleEndian(_partialWriteProbe, 4, 2) + 8;
        if (_partialWriteProbe.size() >= frameSize) {
            _partialWriteProbe.clear();
        }
    }
    return std::nullopt;
}

void UBXReceiverModel::attach(ScriptedReceiver& receiver)
{
    reset(receiver);
}

void UBXReceiverModel::reset(ScriptedReceiver& receiver)
{
    _receiver = &receiver;
    _partialWriteProbe.clear();
    _delayed.clear();
    _identityDelivered = false;
    receiver.setFixedBaudrate(115200);
    receiver.setFatalError(_readError);
    receiver.setReadHandler([this, &receiver](uint8_t*, int,
                                              std::chrono::milliseconds) -> std::optional<GPSReadResult> {
        if (lowLevelProtocolBehavior && surveyPolls > 0 && pollReadError != GPSProtocolError::None) {
            ++failedReads;
            return GPSReadResult{
                pollReadError == GPSProtocolError::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                pollReadDetail};
        }
        if (!receiver.isCancelled() && !_readError) {
            return std::nullopt;
        }
        ++failedReads;
        return GPSReadResult{receiver.isCancelled() ? GPSReadStatus::Cancelled : GPSReadStatus::Error};
    });
}

void UBXReceiverModel::onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout)
{
    Q_UNUSED(receiver)
    Q_UNUSED(timeout)
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
    if (lowLevelProtocolBehavior) {
        return std::min({requested, available, static_cast<int>(readChunk)});
    }
    return std::min({requested, available, coalesceReplies ? requested : 7});
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

QByteArray UBXReceiverModel::_lowLevelIdentityPayload()
{
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
    QByteArray payload;
    payload.append(static_cast<char>(CFG_CLASS));
    payload.append(static_cast<char>(messageId));
    Response response{_frame(0x05, accepted ? 0x01 : 0x00, payload)};
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
    const auto payloadSize = qFromLittleEndian<quint16>(pending.constData() + 4);
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
    if (!(lowLevelProtocolBehavior ? _handleLowLevelFrame(receiver, command) : _handleFrame(receiver, command))) {
        return {GPSWriteStatus::Error};
    }
    const int length = command.size();
    return {GPSWriteStatus::Completed, length, length};
}

std::optional<bool> UBXReceiverModel::handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate)
{
    Q_UNUSED(receiver)
    hostBaud = baudrate;
    if (lowLevelProtocolBehavior) {
        ++transportOperations;
        hostBauds.push_back(baudrate);
    }
    return std::nullopt;
}

QHash<quint32, quint64> UBXReceiverModel::_valsetValues(const QByteArray& payload)
{
    QHash<quint32, quint64> values;
    qsizetype offset = 4;
    while (offset + 4 <= payload.size()) {
        const auto key = qFromLittleEndian<quint32>(payload.constData() + offset);
        offset += 4;
        const unsigned storage = key >> 28;
        if (storage < 1 || storage > 5) {
            wireValid = false;
            break;
        }
        const int size = storage == 1 ? 1 : (1 << (storage - 2));
        if (offset + size > payload.size()) {
            wireValid = false;
            break;
        }
        quint64 value = 0;
        for (int i = 0; i < size; ++i) {
            value |= quint64(static_cast<uint8_t>(payload[offset + i])) << (i * 8);
        }
        values.insert(key, value);
        if (key == TMODE_SVIN_MIN_DUR) {
            surveyDuration = static_cast<unsigned>(value);
        } else if (key == TMODE_SVIN_ACC_LIMIT) {
            surveyAccuracy = static_cast<unsigned>(value);
        } else if (key == TMODE_FIXED_POS_ACC) {
            fixedAccuracy = static_cast<uint32_t>(value);
        } else if (key == NAVSPG_DYNMODEL) {
            navigationModel = static_cast<unsigned>(value);
        }
        offset += size;
    }
    wireValid = wireValid && offset == payload.size();
    return values;
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
    } else if (message == UBX::Msg::CFG_PRT.value()) {
        if (payload.size() != 40 || static_cast<uint8_t>(payload[0]) != PORT_UART1 ||
            static_cast<uint8_t>(payload[20]) != PORT_USB) {
            wireValid = false;
            return false;
        }
        const auto rate = littleEndian(payload, 8, 4);
        if (rate != littleEndian(payload, 28, 4)) {
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
            return _replyToRatePoll(receiver, static_cast<uint16_t>(littleEndian(payload, 0, 2)));
        }
        if (payload.size() != 3) {
            wireValid = false;
            return false;
        }
        const auto output = static_cast<uint16_t>(littleEndian(payload, 0, 2));
        if (output == rateAckMessage) {
            rateFault = rateAck;
        }
        if (rateFault == RateAck::Ignored) {
            return true;
        }
        unsupported = std::ranges::find(unsupportedMessages, output) != unsupportedMessages.end();
        if (!unsupported) {
            messageRates[output] = static_cast<uint8_t>(payload[2]);
        }
    }

    bool reject = message == UBX::Msg::CFG_VALSET.value() || unsupported;
    if (!unsupported && message == UBX::Msg::CFG_MSG.value() &&
        littleEndian(payload, 0, 2) == UBX::Msg::RTCM3_1005.value() && payload.size() > 2 &&
        static_cast<uint8_t>(payload[2]) > 0) {
        ++rtcmEnables;
    }
    if (message == UBX::Msg::CFG_TMODE3.value()) {
        const uint32_t mode = littleEndian(payload, 2, 2);
        modes.push_back(mode);
        legacyFixedAccuracy = littleEndian(payload, 20, 4);
        disabledAt = _clock->nowUs();
        reject = rejectDisable && mode == 0;
    }

    QByteArray ackPayload;
    ackPayload.append(static_cast<char>(message & 0xff));
    ackPayload.append(static_cast<char>(message >> 8));
    const Response ack{_frame(reject ? UBX::Msg::ACK_NAK.value() : UBX::Msg::ACK_ACK.value(),
                              std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ackPayload.constData()),
                                                       static_cast<size_t>(ackPayload.size())))};
    if (rateFault == RateAck::Late) {
        _heldReply = ack;
    } else if (rateFault != RateAck::Lost) {
        _queueResponse(receiver, ack);
    }
    return true;
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

bool UBXReceiverModel::_handleLowLevelFrame(ScriptedReceiver& receiver, const QByteArray& frame)
{
    if (frame.size() < 8) {
        wireValid = false;
        return false;
    }

    const auto message = static_cast<uint16_t>(littleEndian(frame, 2, 2));
    const QByteArray payload = frame.sliced(6, frame.size() - 8);
    const auto payloadSpan = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.constData()),
                                                      static_cast<size_t>(payload.size()));
    if (frame != _frame(message, payloadSpan)) {
        wireValid = false;
        return false;
    }

    if (message == UBX::Msg::MON_VER.value()) {
        identityBauds.push_back(hostBaud);
    }
    if ((message & 0xff) == UBX::MsgClass::CFG && message != UBX::Msg::CFG_VALGET.value() &&
        !(message == UBX::Msg::CFG_TMODE3.value() && payload.isEmpty()) &&
        !(message == UBX::Msg::CFG_MSG.value() && payload.size() == 2)) {
        ++configurationWrites;
        unidentifiedWrites += !_identityDelivered;
    }
    if (receiverBaud != 0 && !usb && hostBaud != receiverBaud) {
        return true;
    }

    if (message == UBX::Msg::MON_COMMS.value()) {
        if (!payload.isEmpty()) {
            wireValid = false;
            return false;
        }
        ++commsPolls;
        return true;
    }
    if (message == UBX::Msg::CFG_TMODE3.value() && payload.isEmpty()) {
        ++readbackRequests;
        switch (readbackReply) {
            case ReadbackReply::Timeout:
                return true;
            case ReadbackReply::AckOnly:
                _queueAck(receiver, UBX::Msg::CFG_TMODE3.id, true);
                return true;
            case ReadbackReply::Nak:
                _queueAck(receiver, UBX::Msg::CFG_TMODE3.id, false);
                return true;
            case ReadbackReply::WriteError:
                return false;
            case ReadbackReply::ReadError:
                _readError = true;
                receiver.setFatalError(true);
                return true;
            case ReadbackReply::Cancelled:
                receiver.cancel();
                return true;
            default:
                break;
        }
        QByteArray response(40, '\0');
        response[0] = static_cast<char>(readbackReply == ReadbackReply::WrongLayer);
        response[2] = static_cast<char>(modes.empty() ? 0 : modes.back());
        QByteArray bytes = _frame(UBX::Msg::CFG_TMODE3.value(),
                                  std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(response.constData()),
                                                           static_cast<size_t>(response.size())));
        if (readbackReply == ReadbackReply::Corrupt) {
            bytes.back() ^= 0x01;
        } else if (readbackReply == ReadbackReply::Truncated) {
            bytes.chop(1);
        }
        _queueResponse(receiver, Response{bytes});
        return true;
    }
    if (message == UBX::Msg::CFG_VALGET.value()) {
        ++readbackRequests;
        if (payload.size() < 8 || payload.size() > 40 || ((payload.size() - 4) % 4) != 0 ||
            payload.first(4) != QByteArray(4, '\0')) {
            wireValid = false;
            return false;
        }
        switch (readbackReply) {
            case ReadbackReply::Timeout:
                return true;
            case ReadbackReply::AckOnly:
                _queueAck(receiver, UBX::Msg::CFG_VALGET.id, true);
                return true;
            case ReadbackReply::Nak:
                _queueAck(receiver, UBX::Msg::CFG_VALGET.id, false);
                return true;
            case ReadbackReply::WriteError:
                return false;
            case ReadbackReply::ReadError:
                _readError = true;
                receiver.setFatalError(true);
                return true;
            case ReadbackReply::Cancelled:
                receiver.cancel();
                return true;
            default:
                break;
        }
        for (qsizetype offset = 4; offset < payload.size(); offset += 4) {
            if (std::ranges::find(unsupportedKeys, littleEndian(payload, offset, 4)) != unsupportedKeys.end()) {
                _queueAck(receiver, UBX::Msg::CFG_VALGET.id, false);
                return true;
            }
        }
        QByteArray response;
        response.append(char{1});
        response.append(static_cast<char>(readbackReply == ReadbackReply::WrongLayer));
        response.append(char{0});
        response.append(char{0});
        for (qsizetype offset = 4; offset < payload.size(); offset += 4) {
            uint32_t key = littleEndian(payload, offset, 4);
            if (readbackReply == ReadbackReply::WrongKey) {
                key ^= 0x01;
            }
            const auto found = currentSettings.find(key);
            uint32_t value = found == currentSettings.end() ? 0 : found->second;
            if (readbackReply == ReadbackReply::WrongValue) {
                value ^= 0x01;
            }
            appendLittleEndian(response, key, 4);
            const unsigned code = key >> 28;
            const unsigned width = code <= 2 ? 1 : 1u << (code - 2);
            appendLittleEndian(response, value, width);
        }
        if (readbackReply == ReadbackReply::WrongPosition) {
            response[2] = 1;
        } else if (readbackReply == ReadbackReply::WrongVersion) {
            response[0] = 2;
        } else if (readbackReply == ReadbackReply::DuplicateValue && response.size() >= 9) {
            response.append(response.sliced(4, 5));
        } else if (readbackReply == ReadbackReply::Truncated) {
            response.chop(1);
        } else if (readbackReply == ReadbackReply::Oversized) {
            response.resize(1024, '\0');
        }
        uint16_t responseMessage = message;
        if (readbackReply == ReadbackReply::WrongMessage) {
            responseMessage = UBX::Msg::CFG_TMODE3.value();
        }
        QByteArray bytes =
            _frame(responseMessage, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(response.constData()),
                                                             static_cast<size_t>(response.size())));
        if (readbackReply == ReadbackReply::Corrupt) {
            bytes.back() ^= 0x01;
        }
        _queueResponse(receiver, Response{bytes});
        return true;
    }

    if (message == UBX::Msg::MON_VER.value()) {
        if (!payload.isEmpty()) {
            wireValid = false;
            return false;
        }
        const QByteArray identity = _lowLevelIdentityPayload();
        QByteArray response = _frame(UBX::Msg::MON_VER.value(),
                                     std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(identity.constData()),
                                                              static_cast<size_t>(identity.size())));
        if (corruptIdentity) {
            response.back() ^= 0x01;
        } else {
            _identityDelivered = true;
        }
        _queueResponse(receiver, Response{response});
        return true;
    }

    if (message == UBX::Msg::NAV_SVIN.value()) {
        if (!payload.isEmpty() || modes.empty() || modes.back() != 0) {
            wireValid = false;
            return false;
        }
        ++surveyPolls;
        if (surveyReplies.isEmpty()) {
            surveyReplies.push_back(SurveyReply::Stopped);
        }
        queueSurveyReply(surveyReplies.at(std::min<qsizetype>(surveyPolls - 1, surveyReplies.size() - 1)));
        return true;
    }

    if (legacy) {
        return _handleLegacyFrame(receiver, message, payload);
    }

    if (message != UBX::Msg::CFG_VALSET.value() || payload.size() < 4) {
        wireValid = false;
        return false;
    }
    if (silencePortConfiguration) {
        return true;
    }

    std::map<uint32_t, uint32_t> settings;
    for (qsizetype offset = 4; offset < payload.size();) {
        const uint32_t key = littleEndian(payload, offset, 4);
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
        settings[key] = littleEndian(payload, offset, width);
        offset += width;
    }

    if (!_heldBaudAck.isEmpty()) {
        queueBytes(_heldBaudAck);
        _heldBaudAck.clear();
        lateBaudAckDelivered = true;
        if (rejectAfterBaudChange) {
            return true;
        }
        if (nakAfterBaudChange) {
            QByteArray ackPayload;
            ackPayload.append(static_cast<char>(message & 0xff));
            ackPayload.append(static_cast<char>(message >> 8));
            _queueResponse(
                receiver,
                Response{_frame(UBX::Msg::ACK_NAK.value(),
                                std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ackPayload.constData()),
                                                         static_cast<size_t>(ackPayload.size())))});
        }
    }

    const bool unsupported = std::ranges::any_of(settings, [this](const auto& setting) {
        return std::ranges::find(unsupportedKeys, setting.first) != unsupportedKeys.end();
    });
    if (unsupported && loseUnsupportedNak) {
        return true;
    }
    bool reject = unsupported;
    if (const auto mode = settings.find(UBX::Cfg::TMODE_MODE.id); mode != settings.end()) {
        modes.push_back(mode->second);
        if (mode->second == 0) {
            disabledAt = _clock->nowUs();
            reject = rejectDisable;
        } else if (mode->second == 1) {
            ++starts;
            startedAt = _clock->nowUs();
            startSettings = settings;
            reject = rejectStart;
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
        for (const auto& setting : settings) {
            currentSettings[setting.first] = setting.second;
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
            QByteArray ackPayload;
            ackPayload.append(static_cast<char>(message & 0xff));
            ackPayload.append(static_cast<char>(message >> 8));
            _heldBaudAck = _frame(UBX::Msg::ACK_ACK.value(),
                                  std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ackPayload.constData()),
                                                           static_cast<size_t>(ackPayload.size())));
            return true;
        }
    }

    QByteArray ackPayload;
    ackPayload.append(static_cast<char>(message & 0xff));
    ackPayload.append(static_cast<char>(message >> 8));
    _queueResponse(receiver,
                   Response{_frame(reject ? UBX::Msg::ACK_NAK.value() : UBX::Msg::ACK_ACK.value(),
                                   std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(ackPayload.constData()),
                                                            static_cast<size_t>(ackPayload.size())))});
    return true;
}

bool UBXReceiverModel::_handleFrame(ScriptedReceiver& receiver, const QByteArray& frame)
{
    const auto messageClass = static_cast<uint8_t>(frame[2]);
    const auto messageId = static_cast<uint8_t>(frame[3]);
    const QByteArray payload = frame.sliced(6, frame.size() - 8);
    if (frame != _frame(messageClass, messageId, payload)) {
        wireValid = false;
        return false;
    }
    if (messageClass == 0x0a && messageId == 0x04 && payload.isEmpty()) {
        QByteArray corruptVersion;
        if (corruptVersionReplies) {
            QByteArray nonBase = _version;
            nonBase.replace(40, 30, padded("FWVER=SPG 3.01", 30));
            corruptVersion = _frame(messageClass, messageId, nonBase);
            corruptVersion.back() ^= 0x01;
            _queueResponse(receiver, Response{corruptVersion});
        }
        _queueResponse(receiver, Response{_frame(messageClass, messageId, _version)});
        if (corruptVersionReplies) {
            _queueResponse(receiver, Response{corruptVersion});
        }
        return true;
    }
    if (messageClass == 0x01 && messageId == 0x3b && payload.isEmpty()) {
        QByteArray status(40, '\0');
        status[37] = static_cast<char>(timeMode == 1 || surveyStopStuck);
        qToLittleEndian<quint32>(retainedSurveyDuration, status.data() + 8);
        ++surveyStopReads;
        _queueResponse(receiver, Response{_frame(messageClass, messageId, status)});
        return true;
    }
    if (messageClass != CFG_CLASS) {
        wireValid = false;
        return false;
    }
    if (messageId == 0x09 || messageId == 0x04) {
        ++resetCommands;
    }
    if (messageId == CFG_VALSET && !_modern) {
        _queueAck(receiver, messageId, false);
        return true;
    }
    if (messageId == CFG_VALGET) {
        if (!_modern || payload.size() != 8 || payload.first(4) != QByteArray(4, '\0')) {
            wireValid = false;
            return false;
        }
        const auto key = qFromLittleEndian<quint32>(payload.constData() + 4);
        QByteArray response = payload;
        response[0] = 1;
        if (key == TMODE_MODE) {
            response.append(static_cast<char>(timeMode));
            ++timeModeReads;
        } else if (key == SEC_JAMDET_SENSITIVITY_HI && delayOptionalNak) {
            _queueAck(receiver, messageId, false);
            return true;
        } else if (const auto setting = currentSettings.find(key);
                   key == SEC_JAMDET_SENSITIVITY_HI && setting != currentSettings.end()) {
            response.append(static_cast<char>(setting->second));
        } else {
            wireValid = false;
            return false;
        }
        return _replyToReadback(receiver, messageId, response, key);
    }
    if (messageId == CFG_TMODE3 && payload.isEmpty()) {
        QByteArray response(40, '\0');
        response[2] = static_cast<char>(timeMode);
        ++timeModeReads;
        return _replyToReadback(receiver, messageId, response, TMODE_MODE);
    }

    std::optional<unsigned> mode;
    if (messageId == CFG_VALSET) {
        const auto values = _valsetValues(payload);
        if (rejectRtcmActivation && values.value(0x30210001) == 1000) {
            _queueAck(receiver, messageId, false);
            return true;
        }
        if (values.contains(TMODE_MODE)) {
            mode = static_cast<unsigned>(values.value(TMODE_MODE));
        }
        if (values.contains(SEC_JAMDET_SENSITIVITY_HI) && delayOptionalAck) {
            _delayNextValsetAck = true;
            if (delayOptionalNak) {
                _queueAck(receiver, messageId, false);
                return true;
            }
        }
        if (values.contains(SEC_JAMDET_SENSITIVITY_HI)) {
            currentSettings[SEC_JAMDET_SENSITIVITY_HI] = static_cast<uint32_t>(values.value(SEC_JAMDET_SENSITIVITY_HI));
        }
    } else if (messageId == CFG_TMODE3 && payload.size() == 40) {
        mode = qFromLittleEndian<quint16>(payload.constData() + 2) & 0xff;
        fixedAccuracy = qFromLittleEndian<quint32>(payload.constData() + 20);
        surveyDuration = qFromLittleEndian<quint32>(payload.constData() + 24);
        surveyAccuracy = qFromLittleEndian<quint32>(payload.constData() + 28);
    } else if (messageId == 0x24 && payload.size() == 36) {
        navigationModel = static_cast<uint8_t>(payload[2]);
    }
    if (rejectRtcmActivation && messageId == 0x08 && payload.size() == 6 &&
        qFromLittleEndian<quint16>(payload.constData()) == 1000) {
        _queueAck(receiver, messageId, false);
        return true;
    }
    if (rejectRtcmActivation && messageId == 0x01 && payload.size() == 3 &&
        static_cast<uint8_t>(payload[0]) == UBX::MsgClass::RTCM3 && payload[2] != 0) {
        _queueAck(receiver, messageId, false);
        return true;
    }
    if (!mode) {
        _queueAck(receiver, messageId, true);
        return true;
    }
    if (*mode == 0) {
        ++disableCommands;
        if (staleDisableAck) {
            _queueAck(receiver, messageId, true);
        }
    }
    if (!_supportsTimeMode || (*mode == 0 && disableReply == DisableReply::Nak)) {
        _queueAck(receiver, messageId, false);
        return true;
    }
    if (*mode != 0) {
        timeMode = *mode;
        _queueAck(receiver, messageId, true);
        return true;
    }
    if (disableReply == DisableReply::Ack || disableReply == DisableReply::Timeout ||
        disableReply == DisableReply::WrongAck || disableReply == DisableReply::CorruptAck) {
        timeMode = 0;
        if (!surveyStopStuck) {
            retainedSurveyDuration = 0;
        }
    }
    return _replyToSetting(receiver, messageId, disableReply);
}

bool UBXReceiverModel::_replyToSetting(ScriptedReceiver& receiver, uint8_t messageId, DisableReply reply)
{
    if (reply == DisableReply::WriteError) {
        return false;
    }
    if (reply == DisableReply::ReadError) {
        _readError = true;
        receiver.setFatalError(true);
        return true;
    }
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
        QByteArray payload;
        payload.append(static_cast<char>(CFG_CLASS));
        payload.append(static_cast<char>(messageId));
        Response response{_frame(0x05, 0x01, payload)};
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
        case ReadbackReply::Cancelled:
            receiver.cancel();
            return true;
    }
    QByteArray response = _frame(CFG_CLASS, messageId, payload);
    if (reply == ReadbackReply::Corrupt) {
        response.back() ^= 0x01;
    }
    _queueResponse(receiver, Response{response});
    return true;
}
