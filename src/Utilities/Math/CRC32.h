#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace QGC {
/// The CRC of each byte value for the reflected CRC-32 polynomial 0xEDB88320.
inline constexpr std::array<uint32_t, 256> CRC32_TABLE = [] {
    std::array<uint32_t, 256> table{};
    for (uint32_t value = 0; value < 256; ++value) {
        uint32_t crc = value;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
        }
        table[value] = crc;
    }
    return table;
}();

static_assert(CRC32_TABLE[0] == 0x00000000 && CRC32_TABLE[127] == 0xc0ba6cad && CRC32_TABLE[128] == 0xedb88320 &&
              CRC32_TABLE[255] == 0x2d02ef8d);

/// Reflected CRC-32 update; the caller supplies the initial state and final complement policy.
[[nodiscard]] constexpr uint32_t crc32Update(std::span<const uint8_t> bytes, uint32_t state = 0)
{
    for (const uint8_t byte : bytes) {
        state = CRC32_TABLE[(state ^ byte) & 0xff] ^ (state >> 8);
    }
    return state;
}
}  // namespace QGC
