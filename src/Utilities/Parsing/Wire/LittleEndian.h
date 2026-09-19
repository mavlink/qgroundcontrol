#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>

namespace LittleEndian {

template <typename T>
concept Scalar = ((std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>) || std::floating_point<T>) &&
                 (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

template <Scalar T>
using Bits = std::conditional_t<
    sizeof(T) == 1, uint8_t,
    std::conditional_t<sizeof(T) == 2, uint16_t, std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>>;

/// Decode an unaligned little-endian scalar, returning nullopt when its bytes are unavailable.
template <Scalar T>
[[nodiscard]] constexpr std::optional<T> read(std::span<const uint8_t> bytes, std::size_t offset = 0)
{
    static_assert(!std::floating_point<T> || std::numeric_limits<T>::is_iec559);
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
        return std::nullopt;
    }

    uint64_t bits = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bits |= uint64_t{bytes[offset + i]} << (8 * i);
    }
    return std::bit_cast<T>(static_cast<Bits<T>>(bits));
}

}  // namespace LittleEndian
