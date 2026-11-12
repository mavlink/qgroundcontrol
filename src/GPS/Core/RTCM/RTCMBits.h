#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/// The bit fields of an RTCM payload, which follow each other most significant bit first, without alignment.
namespace RTCMBits {

/// The @a width bits, at most 64, at bit @a offset of @a payload, which holds them.
[[nodiscard]] constexpr uint64_t field(std::span<const uint8_t> payload, size_t offset, size_t width)
{
    uint64_t value = 0;
    for (size_t bit = offset; bit < offset + width; ++bit) {
        value = (value << 1) | ((payload[bit / 8] >> (7 - bit % 8)) & 1U);
    }
    return value;
}

}  // namespace RTCMBits
