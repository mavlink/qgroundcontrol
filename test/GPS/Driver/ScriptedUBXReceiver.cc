#include "ScriptedUBXReceiver.h"

#include <algorithm>
#include <cstring>

#include <QtCore/QThread>
#include <QtCore/QtEndian>

namespace {
constexpr uint8_t CFG_CLASS = 0x06;
constexpr uint8_t CFG_TMODE3 = 0x71;
constexpr uint8_t CFG_VALSET = 0x8a;
constexpr uint32_t TMODE_MODE = 0x20030001;
constexpr uint32_t TMODE_SVIN_MIN_DUR = 0x40030010;
constexpr uint32_t TMODE_SVIN_ACC_LIMIT = 0x40030011;
constexpr uint32_t NAVSPG_DYNMODEL = 0x20110021;

QByteArray padded(const QByteArray& text, qsizetype size)
{
    QByteArray result = text;
    result.resize(size, '\0');
    return result;
}
}  // namespace

ScriptedUBXReceiver::ScriptedUBXReceiver(Model model, std::atomic_bool& stopRequested)
    : GPSTransport(stopRequested)
    , _stopRequested(stopRequested)
{
    QByteArray hardware = "00080000";
    QByteArray firmware = "HPG 1.40";
    QByteArray module = "NEO-M8P-2";
    switch (model) {
        case Model::M8PBase:
            break;
        case Model::F9P:
            hardware = "00190000";
            firmware = "HPG 1.32";
            module = "ZED-F9P";
            _modern = true;
            break;
        case Model::M8N:
            firmware = "SPG 3.01";
            module = "NEO-M8N";
            _supportsTimeMode = false;
            break;
        case Model::M9N:
            hardware = "00190000";
            firmware = "SPG 4.04";
            module = "NEO-M9N";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Model::M10:
            hardware = "000A0000";
            firmware = "SPG 5.10";
            module = "MAX-M10S";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Model::M8PRover:
            module = "NEO-M8P-0";
            _supportsTimeMode = false;
            break;
        case Model::F9R:
            hardware = "00190000";
            firmware = "HPS 1.30";
            module = "ZED-F9R";
            _modern = true;
            _supportsTimeMode = false;
            break;
        case Model::Unidentified:
            firmware = "UNKNOWN";
            module = "UNKNOWN";
            break;
        case Model::U6:
            hardware = "00040007";
            _supportsTimeMode = false;
            break;
        case Model::M8NEarly:
            _supportsTimeMode = false;
            break;
    }
    _version = padded("EXT CORE", 30) + padded(hardware, 10) + padded("FWVER=" + firmware, 30) +
               padded(_modern ? "PROTVER=27.31" : "PROTVER=20.30", 30) + padded("MOD=" + module, 30);
    if (model == Model::U6) {
        _version.truncate(40);
    } else if (model == Model::M8NEarly) {
        _version = _version.first(40) + padded("PROTVER=15.00", 30);
    }
}

GPSReadResult ScriptedUBXReceiver::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled() || _readError) {
        ++failedReads;
        return {isCancelled() ? GPSReadStatus::Cancelled : GPSReadStatus::Error};
    }
    if (_incoming.isEmpty()) {
        // Simulate a blocking transport timeout, not an unsolicited receiver response.
        if (timeoutMs > 0) {
            QThread::msleep(static_cast<unsigned long>(timeoutMs));
        }
        return {GPSReadStatus::TimedOut};
    }

    auto& response = _incoming.front();
    // Fragment responses across reads, independently of the vendor's packet boundaries.
    const int count = std::min({length, static_cast<int>(response.bytes.size()), 7});
    std::memcpy(buffer, response.bytes.constData(), static_cast<size_t>(count));
    response.bytes.remove(0, count);
    if (response.bytes.isEmpty()) {
        if (response.disableAck) {
            ++disableAcksRead;
        }
        _incoming.removeFirst();
    }
    return {GPSReadStatus::Data, count};
}

GPSWriteResult ScriptedUBXReceiver::write(const uint8_t* buffer, int length)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    _outgoing.append(reinterpret_cast<const char*>(buffer), length);
    while (_outgoing.size() >= 6) {
        const auto payloadSize = qFromLittleEndian<quint16>(_outgoing.constData() + 4);
        const auto frameSize = payloadSize + 8;
        if (_outgoing.size() < frameSize) {
            break;
        }
        const QByteArray frame = _outgoing.first(frameSize);
        _outgoing.remove(0, frameSize);
        if (!_handleFrame(frame)) {
            return {GPSWriteStatus::Error};
        }
    }
    return {GPSWriteStatus::Completed, length, length};
}

QByteArray ScriptedUBXReceiver::_frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload)
{
    QByteArray result = QByteArray::fromHex("b562");
    result.append(static_cast<char>(messageClass));
    result.append(static_cast<char>(messageId));
    result.append(static_cast<char>(payload.size() & 0xff));
    result.append(static_cast<char>((payload.size() >> 8) & 0xff));
    result.append(payload);
    uint8_t a = 0;
    uint8_t b = 0;
    for (qsizetype i = 2; i < result.size(); ++i) {
        a += static_cast<uint8_t>(result[i]);
        b += a;
    }
    result.append(static_cast<char>(a));
    result.append(static_cast<char>(b));
    return result;
}

void ScriptedUBXReceiver::_queueAck(uint8_t messageId, bool accepted, bool disableAck)
{
    QByteArray payload;
    payload.append(static_cast<char>(CFG_CLASS));
    payload.append(static_cast<char>(messageId));
    _incoming.append({_frame(0x05, accepted ? 0x01 : 0x00, payload), disableAck});
}

std::optional<unsigned> ScriptedUBXReceiver::_valsetTimeMode(const QByteArray& payload)
{
    std::optional<unsigned> mode;
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
        if (key == TMODE_MODE) {
            mode = static_cast<unsigned>(value);
        } else if (key == TMODE_SVIN_MIN_DUR) {
            surveyDuration = static_cast<unsigned>(value);
        } else if (key == TMODE_SVIN_ACC_LIMIT) {
            surveyAccuracy = static_cast<unsigned>(value);
        } else if (key == NAVSPG_DYNMODEL) {
            dynamicModel = static_cast<unsigned>(value);
        }
        offset += size;
    }
    wireValid = wireValid && offset == payload.size();
    return mode;
}

bool ScriptedUBXReceiver::_handleFrame(const QByteArray& frame)
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
            _incoming.append({corruptVersion});
        }
        _incoming.append({_frame(messageClass, messageId, _version)});
        if (corruptVersionReplies) {
            _incoming.append({corruptVersion});
        }
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
        _queueAck(messageId, false);
        return true;
    }

    std::optional<unsigned> mode;
    if (messageId == CFG_VALSET) {
        mode = _valsetTimeMode(payload);
    } else if (messageId == CFG_TMODE3 && payload.size() == 40) {
        mode = qFromLittleEndian<quint16>(payload.constData() + 2) & 0xff;
        surveyDuration = qFromLittleEndian<quint32>(payload.constData() + 24);
        surveyAccuracy = qFromLittleEndian<quint32>(payload.constData() + 28);
    } else if (messageId == 0x24 && payload.size() == 36) {
        dynamicModel = static_cast<uint8_t>(payload[2]);
    }
    if (!mode) {
        _queueAck(messageId, true);
        return true;
    }
    if (*mode == 0) {
        ++disableCommands;
        lastDisablePayload = payload;
    }
    if (!_supportsTimeMode || (*mode == 0 && disableReply == DisableReply::Nak)) {
        _queueAck(messageId, false);
        return true;
    }
    if (*mode != 0) {
        timeMode = *mode;
        _queueAck(messageId, true);
        return true;
    }
    if (disableReply == DisableReply::WriteError) {
        return false;
    }
    if (disableReply == DisableReply::ReadError) {
        _readError = true;
        return true;
    }
    if (disableReply == DisableReply::Cancelled) {
        _stopRequested.store(true);
        return true;
    }

    timeMode = 0;
    if (disableReply == DisableReply::Timeout) {
        return true;
    }
    if (disableReply == DisableReply::WrongAck) {
        _queueAck(0x01, true);
        return true;
    }
    _queueAck(messageId, true, disableReply == DisableReply::Ack);
    if (disableReply == DisableReply::CorruptAck) {
        _incoming.last().bytes.back() ^= 0x01;
    }
    return true;
}
