#include "GPSEvidenceTransport.h"

#include <optional>
#include <utility>

#include "GPSReceiverConfig.h"
#include "Protocols/Support/ProtocolTestPackets.h"

namespace {
std::optional<quint64> keyValue(const QByteArray& payload, quint32 requestedKey)
{
    qsizetype offset = 4;
    while (offset + 4 <= payload.size()) {
        const quint32 key =
            LittleEndian::read<uint32_t>(GPSTest::bytesOf(payload), static_cast<size_t>(offset)).value_or(0);
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

GPSEvidenceTransport::GPSEvidenceTransport(GPSTransport& transport, GPSCancelToken cancelToken)
    : GPSTransport(std::move(cancelToken))
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

GPSReadResult GPSEvidenceTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    const auto result = _transport.read(buffer, timeout);
    if (result.status == GPSReadStatus::Data && result.bytesRead > 0 &&
        static_cast<size_t>(result.bytesRead) <= buffer.size()) {
        _observe(_incoming, buffer.first(static_cast<size_t>(result.bytesRead)), true);
    }
    return result;
}

GPSWriteResult GPSEvidenceTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    const auto result = _transport.write(bytes, deadline);
    _recordWrite(bytes, result);
    return result;
}

std::chrono::milliseconds GPSEvidenceTransport::configurationWriteTimeout() const
{
    return _transport.configurationWriteTimeout();
}

void GPSEvidenceTransport::_recordWrite(QByteArrayView bytes, const GPSWriteResult& result)
{
    _acceptedBytes += result.acceptedBytes;
    _writtenBytes += result.writtenBytes;
    if (result.status == GPSWriteStatus::Completed && result.writtenBytes == bytes.size() &&
        result.acceptedBytes == bytes.size() && result.uncertainBytes() == 0) {
        _observe(_outgoing, bytes, false);
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

QJsonArray GPSEvidenceTransport::requestedSettings(const GPSReceiverConfig& config, bool passive) const
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
    const bool fixed = std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode);
    setting("time_mode", !passive ? (fixed ? 2 : 1) : 0, 0x20030001, 0x71, 2, 1);
    if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode); !passive && survey) {
        setting("survey_duration_s", static_cast<quint64>(survey->duration.count()), 0x40030010, 0x71, 24, 4);
        setting("survey_accuracy_0.1mm", static_cast<quint64>(survey->accuracyMeters * 10000), 0x40030011, 0x71, 28, 4);
    }
    return result;
}

void GPSEvidenceTransport::_observe(QByteArray& pending, QByteArrayView bytes, bool incoming)
{
    pending.append(bytes);
    while (pending.size() >= 2) {
        if (static_cast<uint8_t>(pending[0]) != 0xb5 || static_cast<uint8_t>(pending[1]) != 0x62) {
            pending.remove(0, 1);
            continue;
        }
        if (pending.size() < 6) {
            return;
        }
        const int payloadSize = LittleEndian::read<uint16_t>(GPSTest::bytesOf(pending), 4).value_or(0);
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
