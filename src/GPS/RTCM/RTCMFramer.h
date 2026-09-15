#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

/// Bounded RTCM framing shared by receiver codecs and correction inputs.
/// Drain nextFrame() after completion to recover buffered suffixes.
class RTCMFramer
{
public:
    static constexpr uint8_t PREAMBLE = 0xd3;
    static constexpr uint16_t HEADER_SIZE = 3;
    static constexpr uint16_t CRC_SIZE = 3;
    static constexpr uint16_t MAX_PAYLOAD_SIZE = 1023;
    static constexpr uint16_t MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE;

    void reset()
    {
        _size = 0;
        _payloadSize = 0;
        _frameSize = 0;
        _recoverySize = 0;
    }

    bool addByte(uint8_t byte)
    {
        if (_frameSize) {
            _discardCandidate();
        }
        if (!_size && byte != PREAMBLE) {
            return false;
        }
        _bytes[_size++] = byte;
        return _inspect();
    }

    bool nextFrame()
    {
        if (!_frameSize) {
            return false;
        }
        _discardCandidate();
        return _inspect();
    }

    bool hasPartialFrame() const { return _size != 0 && !_frameSize; }

    /// Complete candidate, including malformed headers; empty while incomplete.
    std::span<const uint8_t> frame() const { return {_bytes.data(), _frameSize}; }

    /// Retained suffix length, including bytes beyond the current candidate.
    uint16_t bufferedSize() const { return _size; }

    uint8_t* message() { return _bytes.data(); }

    const uint8_t* message() const { return _bytes.data(); }

    uint16_t messageLength() const { return _frameSize ? _frameSize : _size; }

    uint16_t payloadLength() const { return _payloadSize; }

    uint16_t messageId() const
    {
        return messageLength() >= 5 && payloadLength() >= 2 ? (_bytes[3] << 4) | (_bytes[4] >> 4) : 0;
    }

    bool valid() const { return isValidFrame(frame()); }

    static bool isValidFrame(std::span<const uint8_t> bytes)
    {
        if (bytes.size() < HEADER_SIZE + 2 + CRC_SIZE || bytes[0] != PREAMBLE || (bytes[1] & 0xfc)) {
            return false;
        }
        const size_t payloadSize = ((bytes[1] & 3) << 8) | bytes[2];
        return bytes.size() == HEADER_SIZE + payloadSize + CRC_SIZE && crc24q(bytes) == 0;
    }

    static uint32_t crc24q(std::span<const uint8_t> bytes)
    {
        uint32_t crc = 0;
        for (const uint8_t byte : bytes) {
            crc ^= uint32_t(byte) << 16;
            for (int bit = 0; bit < 8; ++bit) {
                crc <<= 1;
                if (crc & 0x1000000) {
                    crc ^= 0x1864cfb;
                }
            }
        }
        return crc & 0xffffff;
    }

private:
    void _discardPrefix(uint16_t count)
    {
        if (count) {
            std::move(_bytes.begin() + count, _bytes.begin() + _size, _bytes.begin());
            _size -= count;
            _recoverySize -= (std::min) (_recoverySize, count);
        }
    }

    void _discardCandidate()
    {
        uint16_t searchOffset = _frameSize;
        if (!valid()) {
            _recoverySize = (std::max) (_recoverySize, _frameSize);
            searchOffset = 1;
        }
        uint16_t discardSize = _frameSize;
        if (searchOffset < _recoverySize) {
            discardSize = _recoverySize;
            // Fresh bytes may belong to an incomplete outer frame.
            const std::span<const uint8_t> bytes{_bytes.data(), _recoverySize};
            for (uint16_t offset = searchOffset; offset < _recoverySize; ++offset) {
                const auto suffix = bytes.subspan(offset);
                if (suffix[0] != PREAMBLE || (suffix.size() >= 2 && (suffix[1] & 0xfc))) {
                    continue;
                }
                if (suffix.size() < HEADER_SIZE) {
                    discardSize = (std::min) (discardSize, offset);
                    continue;
                }
                const size_t payloadSize = ((suffix[1] & 3) << 8) | suffix[2];
                if (payloadSize < 2) {
                    continue;
                }
                const size_t length = HEADER_SIZE + payloadSize + CRC_SIZE;
                if (length > suffix.size()) {
                    discardSize = (std::min) (discardSize, offset);
                    continue;
                }
                if (isValidFrame(suffix.first(length))) {
                    discardSize = offset;
                    break;
                }
            }
        }
        _discardPrefix(discardSize);
        _frameSize = 0;
    }

    bool _inspect()
    {
        _frameSize = 0;
        _payloadSize = 0;
        const auto preamble = std::find(_bytes.begin(), _bytes.begin() + _size, PREAMBLE);
        _discardPrefix(static_cast<uint16_t>(preamble - _bytes.begin()));
        if (_size >= 2 && (_bytes[1] & 0xfc)) {
            _frameSize = 2;
            return true;
        }
        if (_size < HEADER_SIZE) {
            return false;
        }
        _payloadSize = ((_bytes[1] & 3) << 8) | _bytes[2];
        if (_payloadSize < 2) {
            _frameSize = HEADER_SIZE;
            return true;
        }
        const uint16_t expectedSize = HEADER_SIZE + _payloadSize + CRC_SIZE;
        if (_size < expectedSize) {
            return false;
        }
        _frameSize = expectedSize;
        return true;
    }

    std::array<uint8_t, MAX_FRAME_SIZE> _bytes{};
    uint16_t _size = 0;
    uint16_t _payloadSize = 0;
    uint16_t _frameSize = 0;
    uint16_t _recoverySize = 0;
};
