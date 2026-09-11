#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

/// Bounded RTCM framing shared by receiver codecs and correction inputs.
/// Completion includes invalid frames so callers can retain diagnostic evidence.
class RTCMFramer
{
public:
    static constexpr uint8_t PREAMBLE = 0xd3;
    static constexpr uint16_t HEADER_SIZE = 3;
    static constexpr uint16_t CRC_SIZE = 3;
    static constexpr uint16_t MAX_PAYLOAD_SIZE = 1023;

    void reset()
    {
        _size = 0;
        _payloadSize = 0;
        _complete = false;
    }

    bool addByte(uint8_t byte)
    {
        if (_complete) {
            reset();
        }
        if (!_size && byte != PREAMBLE) {
            return false;
        }
        _bytes[_size++] = byte;
        if (_size == HEADER_SIZE) {
            _payloadSize = ((_bytes[1] & 3) << 8) | _bytes[2];
            if (!_payloadSize) {
                reset();
                return false;
            }
        }
        _complete = _size >= HEADER_SIZE && _size == HEADER_SIZE + _payloadSize + CRC_SIZE;
        return _complete;
    }

    bool hasPartialFrame() const { return _size != 0 && !_complete; }

    uint8_t* message() { return _bytes.data(); }

    const uint8_t* message() const { return _bytes.data(); }

    uint16_t messageLength() const { return _size; }

    uint16_t payloadLength() const { return _payloadSize; }

    uint16_t messageId() const { return _size >= 5 && payloadLength() >= 2 ? (_bytes[3] << 4) | (_bytes[4] >> 4) : 0; }

    bool valid() const { return _complete && isValidFrame({_bytes.data(), _size}); }

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
    std::array<uint8_t, HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE> _bytes{};
    uint16_t _size = 0;
    uint16_t _payloadSize = 0;
    bool _complete = false;
};
