#include "GPSEvidenceTransport.h"

#include <optional>

#include <QtCore/QtEndian>

#include "GPSReceiverConfig.h"

namespace {
std::optional<quint64> keyValue(const QByteArray& payload, quint32 requestedKey)
{
    qsizetype offset = 4;
    while (offset + 4 <= payload.size()) {
        const quint32 key = qFromLittleEndian<quint32>(payload.constData() + offset);
        offset += 4;
        const unsigned storage = key >> 28;
        if (storage < 1 || storage > 5) {
            return {};
        }
        const int size = storage == 1 ? 1 : (1 << (storage - 2));
        if (offset + size > payload.size()) {
            return {};
        }
        quint64 value = 0;
        for (int i = 0; i < size; ++i) {
            value |= quint64(static_cast<uint8_t>(payload[offset + i])) << (8 * i);
        }
        if (key == requestedKey) {
            return value;
        }
        offset += size;
    }
    return {};
}
}  // namespace

GPSEvidenceTransport::GPSEvidenceTransport(GPSTransport& transport, const std::atomic_bool& stop)
    : GPSTransport(stop)
    , _transport(transport)
{}

GPSOpenResult GPSEvidenceTransport::open()
{
    return _transport.open();
}

bool GPSEvidenceTransport::fatalError() const
{
    return _transport.fatalError();
}

unsigned GPSEvidenceTransport::fixedBaudrate() const
{
    return _transport.fixedBaudrate();
}

bool GPSEvidenceTransport::setBaudrate(unsigned baudrate)
{
    return _transport.setBaudrate(baudrate);
}

GPSReadResult GPSEvidenceTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    const auto result = _transport.read(buffer, length, timeoutMs);
    if (result.status == GPSReadStatus::Data && result.bytesRead > 0 && result.bytesRead <= length) {
        _observe(_incoming, buffer, result.bytesRead, true);
    }
    return result;
}

GPSWriteResult GPSEvidenceTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    const auto result = _transport.writeBounded(buffer, length, deadline);
    _recordWrite(buffer, length, result);
    return result;
}

GPSWriteResult GPSEvidenceTransport::writeConfiguration(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    const auto result = _transport.writeConfiguration(buffer, length, deadline);
    _recordWrite(buffer, length, result);
    return result;
}

std::chrono::milliseconds GPSEvidenceTransport::configurationWriteTimeout() const
{
    return _transport.configurationWriteTimeout();
}

std::chrono::milliseconds GPSEvidenceTransport::correctionWriteTimeout(int length) const
{
    return _transport.correctionWriteTimeout(length);
}

void GPSEvidenceTransport::_recordWrite(const uint8_t* data, int length, const GPSWriteResult& result)
{
    _acceptedBytes += result.acceptedBytes;
    _writtenBytes += result.writtenBytes;
    if (result.status == GPSWriteStatus::Completed && result.writtenBytes == length && result.acceptedBytes == length &&
        result.uncertainBytes() == 0) {
        _observe(_outgoing, data, length, false);
    } else {
        ++_failedWrites;
        _outgoing.clear();
    }
}

QJsonObject GPSEvidenceTransport::evidence() const
{
    return {{"transport_accepted_bytes", _acceptedBytes},
            {"transport_written_bytes", _writtenBytes},
            {"failed_writes", _failedWrites},
            {"ubx_ack_frames", _ackCount},
            {"ubx_nak_frames", _nakCount},
            {"corrupt_ubx_frames", _corruptFrames},
            {"omitted_frames", _omittedFrames},
            {"frames", _frames},
            {"interpretation",
             "Passive observation only; ACK class/id may be stale or ambiguous. "
             "Written bytes are not receiver acknowledgement."}};
}

QJsonArray GPSEvidenceTransport::requestedSettings(const GPSReceiverConfig& config) const
{
    QJsonArray result;
    auto setting = [&](const QString& name, quint64 requested, quint32 key, int legacyId, int legacyOffset,
                       int legacySize) {
        bool written = false;
        bool ack = false;
        std::optional<quint64> readback;
        for (const auto& entry : _frames) {
            const auto frame = entry.toObject();
            const bool incoming = frame["direction"] == "rx";
            const int messageClass = frame["class"].toInt();
            const int messageId = frame["id"].toInt();
            const QByteArray payload = QByteArray::fromHex(frame["payload_hex"].toString().toLatin1());
            if (incoming && messageClass == 5 && messageId == 1 && payload.size() == 2 && payload[0] == 6 &&
                (static_cast<uint8_t>(payload[1]) == 0x8a || static_cast<uint8_t>(payload[1]) == legacyId)) {
                ack = true;
            }
            if (messageClass != 6) {
                continue;
            }
            std::optional<quint64> value;
            if ((!incoming && messageId == 0x8a) || (incoming && messageId == 0x8b && payload.size() >= 4 &&
                                                     payload.first(4) == QByteArray::fromHex("01000000"))) {
                value = keyValue(payload, key);
            } else if (messageId == legacyId && legacyOffset >= 0 && payload.size() >= legacyOffset + legacySize) {
                quint64 number = 0;
                for (int i = 0; i < legacySize; ++i) {
                    number |= quint64(static_cast<uint8_t>(payload[legacyOffset + i])) << (8 * i);
                }
                value = number;
            }
            if (value) {
                if (incoming) {
                    readback = value;
                } else {
                    written = written || *value == requested;
                    readback.reset();
                }
            }
        }
        QJsonObject evidence{{"setting", name},
                             {"requested_wire_value", static_cast<qint64>(requested)},
                             {"matching_write_observed", written},
                             {"ack_with_same_message_type_observed", ack},
                             {"readback", !readback                ? "not_observed"
                                          : *readback == requested ? "matching_value_observed"
                                                                   : "different_value_observed"},
                             {"transaction_correlation_verified", false}};
        if (readback) {
            evidence.insert("readback_wire_value", static_cast<qint64>(*readback));
        }
        result.append(evidence);
    };
    setting("time_mode", config.role == GPSReceiverConfig::Role::RTKBase ? 1 : 0, 0x20030001, 0x71, 2, 1);
    if (config.role == GPSReceiverConfig::Role::RTKBase) {
        setting("survey_duration_s", config.base.surveyInDurationSecs, 0x40030010, 0x71, 24, 4);
        setting("survey_accuracy_0.1mm", static_cast<quint64>(config.base.surveyInAccMeters * 10000), 0x40030011, 0x71,
                28, 4);
    }
    if (config.dynamicModel) {
        setting("dynamic_model", *config.dynamicModel, 0x20110021, 0x24, 2, 1);
    }
    if (config.constellationMask) {
        setting("gps_enabled", (config.constellationMask & 1) != 0, 0x1031001f, -1, -1, 0);
        setting("sbas_enabled", (config.constellationMask & 2) != 0, 0x10310020, -1, -1, 0);
        setting("galileo_enabled", (config.constellationMask & 4) != 0, 0x10310021, -1, -1, 0);
        setting("beidou_enabled", (config.constellationMask & 8) != 0, 0x10310022, -1, -1, 0);
        setting("glonass_enabled", (config.constellationMask & 16) != 0, 0x10310025, -1, -1, 0);
    }
    return result;
}

void GPSEvidenceTransport::_observe(QByteArray& pending, const uint8_t* data, int length, bool incoming)
{
    pending.append(reinterpret_cast<const char*>(data), length);
    while (pending.size() >= 2) {
        if (static_cast<uint8_t>(pending[0]) != 0xb5 || static_cast<uint8_t>(pending[1]) != 0x62) {
            pending.remove(0, 1);
            continue;
        }
        if (pending.size() < 6) {
            return;
        }
        const int payloadSize = qFromLittleEndian<quint16>(pending.constData() + 4);
        if (payloadSize > 8192) {
            pending.remove(0, 2);
            continue;
        }
        const int frameSize = payloadSize + 8;
        if (pending.size() < frameSize) {
            return;
        }
        uint8_t a = 0;
        uint8_t b = 0;
        for (int i = 2; i < frameSize - 2; ++i) {
            a += static_cast<uint8_t>(pending[i]);
            b += a;
        }
        if (a != static_cast<uint8_t>(pending[frameSize - 2]) || b != static_cast<uint8_t>(pending[frameSize - 1])) {
            ++_corruptFrames;
            pending.remove(0, 2);
            continue;
        }
        const auto messageClass = static_cast<uint8_t>(pending[2]);
        const auto messageId = static_cast<uint8_t>(pending[3]);
        const QByteArray payload = pending.sliced(6, payloadSize);
        pending.remove(0, frameSize);
        if (incoming && messageClass == 5 && payloadSize == 2) {
            _ackCount += messageId == 1;
            _nakCount += messageId == 0;
        }
        // Keep only configuration, ACK and identity evidence, not unbounded navigation traffic.
        if (messageClass != 5 && messageClass != 6 && messageClass != 10) {
            continue;
        }
        if (_frames.size() >= 256) {
            ++_omittedFrames;
            continue;
        }
        _frames.append(QJsonObject{{"direction", incoming ? "rx" : "tx"},
                                   {"class", messageClass},
                                   {"id", messageId},
                                   {"payload_hex", QString::fromLatin1(payload.toHex())}});
    }
}
