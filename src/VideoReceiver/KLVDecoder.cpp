#include "KLVDecoder.h"
#include <optional>
#include <cstring>
#include <QDebug>

namespace {
constexpr uint8_t UAS_LOCAL_SET_KEY[16] {0x06, 0x0E, 0x2B, 0x34, 0x02, 0x0B, 0x01, 0x01, 0x0E, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00,
                                        0x00};

std::optional<std::vector<uint8_t>> decodeBERField(uint8_t *data, size_t &i, size_t dataSize) {
    // If the first bit is zzeo, we have short form length encoding and return the number
    if (!(data[i] & (1 << 7))) {
        return std::vector<uint8_t> {data[i++],};
    }

    uint8_t length = data[i++];

    if (i + length > dataSize) {
        return {};
    }

    std::vector<uint8_t> res(length);
    std::memcpy(res.data(), data + i, length);
    i += length;
    return res;
}


std::optional<uint64_t> decodeBERNumber(uint8_t *data, size_t &i, size_t dataSize) {
    auto berField = decodeBERField(data, i, dataSize);
    if (berField.has_value()) {
        uint64_t res = 0;
        // We could return nothing if data field is longer than 4 bytes (which dhouldn't happen), but in order to avoid it we just use last 8 bytes
        for (const auto &v: berField.value()) {
            res = (res << 8) | v;
        }
        return res;
    }
    return {};
}

std::optional<uint32_t> decodeBEROIDTag(uint8_t *data, size_t &i, size_t dataSize) {
    // FIXME: currently only one-byte tags are used, but in furute a BER_OID tag should be parsed here
    return data[i++];
}
}

std::unordered_map<uint32_t, std::vector<uint8_t>> KLVDecoder::getAllMetadata() {
    return _lastMetadata;
}

std::optional<uint64_t> KLVDecoder::getTimestamp() {
    if (_lastMetadata.find(2) != _lastMetadata.end()) {
        return _decodeUint64(_lastMetadata[2]);
    }
    return {};
}

std::optional<QString> KLVDecoder::getMissionID() {
    if (_lastMetadata.find(3) != _lastMetadata.end()) {
        return _decodeString(_lastMetadata[3]);
    }
    return {};
}


std::optional<QString> KLVDecoder::getImageSensor() {
    if (_lastMetadata.find(11) != _lastMetadata.end()) {
        return _decodeString(_lastMetadata[11]);
    }
    return {};
}

uint64_t KLVDecoder::_decodeUint64(const std::vector<uint8_t> &value) {
    uint64_t res = 0;
    for (auto const &byte: value) {
        res = (res << 8) | byte;
    }
    return res;
}

QString KLVDecoder::_decodeString(const std::vector<uint8_t> &value) {
    QString res;
    res.reserve(value.size() + 1);
    for (auto const &byte: value) {
        res.append(static_cast<QChar>(byte));
    }
    return res;
}


void KLVDecoder::decode(uint8_t *data, size_t size) {
    // Remove all the preious records
    _lastMetadata.clear();

    // If the size if smaller then the key and packet size, there is no reason to decode anything
    if (size < sizeof(UAS_LOCAL_SET_KEY) + 1) {
        qDebug() << "Packet is too small";
        return;
    }

    // The first 16 bytes should contain the valid UAS local set key
    if (memcmp(UAS_LOCAL_SET_KEY, data, sizeof(UAS_LOCAL_SET_KEY)) != 0) {
        qDebug() << "Key does not match";
        return;
    }

    // Initializa current index with the next value after the key
    size_t i = sizeof(UAS_LOCAL_SET_KEY);
    // The next field is always packet length in BER encoding
    auto packetLength = decodeBERNumber(data, i, size);

    if (!packetLength.has_value()) {
        return;
    }

    // If the data is longer then packet length, trim it to fit packet length
    size = std::min(size, packetLength.value() + i);

    while (i < size) {

        auto key = decodeBEROIDTag(data, i, size);
        if (!key.has_value()) {
            return;
        }

        auto length = decodeBERNumber(data, i, size);
        if (!length.has_value()) {
            return;
        }

        if (i + length.value() > size) {
            return;
        }

        _lastMetadata[key.value()] = std::vector<uint8_t>(length.value());
        std::memcpy(_lastMetadata[key.value()].data(), data + i, length.value());
        i += length.value();
    }

    // TODO: check whether the last element was a checksum and not update the map values until it is checked
}
