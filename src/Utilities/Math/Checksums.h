#pragma once

#include <cstdint>
#include <span>

namespace QGC {

/// UBX checksum pair: an 8-bit Fletcher sum over the class, ID, length and payload bytes.
struct Fletcher8
{
    uint8_t a = 0;
    uint8_t b = 0;

    constexpr bool operator==(const Fletcher8&) const = default;
};

/// Continues @a state over @a bytes, so a frame checksummed in parts matches the contiguous result.
[[nodiscard]] constexpr Fletcher8 fletcher8(std::span<const uint8_t> bytes, Fletcher8 state = {})
{
    for (const uint8_t byte : bytes) {
        state.a = static_cast<uint8_t>(state.a + byte);
        state.b = static_cast<uint8_t>(state.b + state.a);
    }
    return state;
}

/// SBF's CRC-16-CCITT: polynomial 0x1021, initial value 0, no reflection and no final XOR (CRC-16/XMODEM).
[[nodiscard]] constexpr uint16_t crc16Ccitt(std::span<const uint8_t> bytes)
{
    uint16_t crc = 0;
    for (const uint8_t byte : bytes) {
        uint8_t x = static_cast<uint8_t>((crc >> 8) ^ byte);
        x ^= x >> 4;
        crc = static_cast<uint16_t>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
    }
    return crc;
}

/// RTCM 3's CRC-24Q: polynomial 0x1864CFB, initial value 0, no reflection and no final XOR.
[[nodiscard]] constexpr uint32_t crc24q(std::span<const uint8_t> bytes)
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

/// NMEA 0183 checksum: the XOR of the sentence body between '$' and '*'.
[[nodiscard]] constexpr uint8_t nmeaChecksum(std::span<const uint8_t> body)
{
    uint8_t result = 0;
    for (const uint8_t byte : body) {
        result ^= byte;
    }
    return result;
}

}  // namespace QGC
