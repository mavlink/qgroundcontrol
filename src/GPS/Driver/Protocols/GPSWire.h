#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>

namespace GPSWire {
template <typename T>
concept Scalar = (std::integral<T> && !std::same_as<T, bool>) || std::floating_point<T>;

template <Scalar T>
using Bits = std::conditional_t<
    sizeof(T) == 1, uint8_t,
    std::conditional_t<sizeof(T) == 2, uint16_t, std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>>;

template <Scalar T>
std::optional<T> read(std::span<const uint8_t> bytes, size_t offset = 0)
{
    static_assert(sizeof(T) == sizeof(Bits<T>));
    static_assert(!std::floating_point<T> || std::numeric_limits<T>::is_iec559);
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
        return std::nullopt;
    }
    Bits<T> bits = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        bits |= static_cast<Bits<T>>(bytes[offset + i]) << (8 * i);
    }
    return std::bit_cast<T>(bits);
}

template <Scalar T>
bool write(std::span<uint8_t> bytes, size_t offset, T value)
{
    static_assert(sizeof(T) == sizeof(Bits<T>));
    static_assert(!std::floating_point<T> || std::numeric_limits<T>::is_iec559);
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
        return false;
    }
    const auto bits = std::bit_cast<Bits<T>>(value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        bytes[offset + i] = static_cast<uint8_t>(bits >> (8 * i));
    }
    return true;
}
}  // namespace GPSWire
